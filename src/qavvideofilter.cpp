/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideofilter_p.h"
#include "qavfilter_p_p.h"
#include "qavcodec_p.h"
#include "qavvideoframe.h"
#include "qavstream.h"
#include <QDebug>

extern "C" {
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/avassert.h>
#include <libavutil/bprint.h>
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

class QAVVideoFilterPrivate : public QAVFilterPrivate
{
public:
    QAVVideoFilterPrivate(QAVFilter *q, QMutex &mutex) : QAVFilterPrivate(q, mutex) { }

    QList<QAVVideoInputFilter> inputs;
    QList<QAVVideoOutputFilter> outputs;
};

QAVVideoFilter::QAVVideoFilter(
    const QAVStream &stream,
    const QString &name,
    const QList<QAVVideoInputFilter> &inputs,
    const QList<QAVVideoOutputFilter> &outputs,
    QMutex &mutex)
    : QAVFilter(
        stream,
        name,
        *new QAVVideoFilterPrivate(this, mutex))
{
    Q_D(QAVVideoFilter);
    d->inputs = inputs;
    d->outputs = outputs;
}

int QAVVideoFilter::write(const QAVFrame &frame)
{
    Q_D(QAVVideoFilter);
    if (!frame || frame.stream().stream()->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
        qWarning() << "Frame is not video";
        return AVERROR(EINVAL);
    }
    if (!d->isEmpty)
        return AVERROR(EAGAIN);

    d->sourceFrame = frame;
    for (const auto &filter : d->inputs) {
        if (!filter.supports(d->sourceFrame)) {
            d->sourceFrame = {};
            return AVERROR(ENOTSUP);
        }
        QAVFrame ref = d->sourceFrame;
        QMutexLocker locker(&d->graphMutex);
        int ret = av_buffersrc_add_frame_flags(filter.ctx(), ref.frame(), AV_BUFFERSRC_FLAG_PUSH);
        if (ret < 0)
            return ret;
    }

    d->isEmpty = false;
    return 0;
}

int QAVVideoFilter::read(QAVFrame &frame)
{
    Q_D(QAVVideoFilter);
    if (d->outputs.isEmpty() || d->isEmpty) {
        int ret = AVERROR(EAGAIN);
        if (d->sourceFrame && d->outputs.isEmpty()) {
            frame = d->sourceFrame;
            ret = 0;
        }
        d->sourceFrame = {};
        d->isEmpty = true;
        return ret;
    }

    int ret = 0;
    if (d->outputFrames.isEmpty()) {
        for (int i = 0; i < d->outputs.size(); ++i) {
            const auto &filter = d->outputs[i];
            while (true) {
                QAVFrame out = d->sourceFrame;
                // av_buffersink_get_frame_flags allocates frame's data
                av_frame_unref(out.frame());
                {
                    QMutexLocker locker(&d->graphMutex);
                    ret = av_buffersink_get_frame_flags(filter.ctx(), out.frame(), 0);
                }
                if (ret < 0)
                    break;

#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 30, 0)
                if (!out.frame()->pkt_duration)
                    out.frame()->pkt_duration = d->sourceFrame.frame()->pkt_duration;
#else
                if (out.frame()->duration == AV_NOPTS_VALUE || out.frame()->duration == 0)
                    out.frame()->duration = d->sourceFrame.frame()->duration;
#endif
                out.setFrameRate(av_buffersink_get_frame_rate(filter.ctx()));
                out.setTimeBase(av_buffersink_get_time_base(filter.ctx()));
                out.setFilterName(
                    !filter.name().isEmpty()
                    ? filter.name()
                    : QString(QLatin1String("%1:%2")).arg(d->name).arg(QString::number(i)));
                if (!out.stream())
                    out.setStream(d->stream);
                d->outputFrames.push_back(out);
            }
        }
    }

    ret = AVERROR(EAGAIN);
    if (!d->outputFrames.isEmpty()) {
        frame = d->outputFrames.takeFirst();
        ret = 0;
    }
    if (d->outputFrames.isEmpty()) {
        d->sourceFrame = {};
        d->isEmpty = true;
    }
    return ret;
}

void QAVVideoFilter::flush()
{
    Q_D(QAVVideoFilter);
    for (const auto &filter : d->inputs) {
        int ret = av_buffersrc_add_frame(filter.ctx(), nullptr);
        if (ret < 0)
            qWarning() << "Could not flush:" << ret;
    }
    d->isEmpty = false;
}

QT_END_NAMESPACE
