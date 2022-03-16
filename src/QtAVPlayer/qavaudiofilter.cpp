/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiofilter_p.h"
#include "qavfilter_p_p.h"
#include "qavcodec_p.h"
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

class QAVAudioFilterPrivate : public QAVFilterPrivate
{
public:
    QAVAudioFilterPrivate(QAVFilter *q) : QAVFilterPrivate(q) { }

    QList<QAVAudioInputFilter> inputs;
    QList<QAVAudioOutputFilter> outputs;
};

QAVAudioFilter::QAVAudioFilter(const QList<QAVAudioInputFilter> &inputs, const QList<QAVAudioOutputFilter> &outputs, QObject *parent)
    : QAVFilter(*new QAVAudioFilterPrivate(this), parent)
{
    Q_D(QAVAudioFilter);
    d->inputs = inputs;
    d->outputs = outputs;
}

int QAVAudioFilter::write(const QAVFrame &frame)
{
    Q_D(QAVAudioFilter);
    if (frame.stream().stream()->codecpar->codec_type != AVMEDIA_TYPE_AUDIO) {
        qWarning() << "Frame is not audio";
        return AVERROR(EINVAL);
    }

    d->sourceFrame = frame;
    for (auto &filter : d->inputs) {
        QAVFrame ref = frame;
        int ret = av_buffersrc_add_frame_flags(filter.ctx(), ref.frame(), 0);
        if (ret < 0)
            return ret;
    }

    return 0;
}

int QAVAudioFilter::read(QAVFrame &frame)
{
    Q_D(QAVAudioFilter);
    if (d->outputs.isEmpty() || !d->sourceFrame) {
        frame = d->sourceFrame;
        d->sourceFrame = {};
        return 0;
    }

    int ret = 0;
    if (d->outputFrames.isEmpty()) {
        for (auto &filter: d->outputs) {
            while (true) {
                QAVFrame out = d->sourceFrame;
                // av_buffersink_get_frame_flags allocates frame's data
                av_frame_unref(out.frame());
                ret = av_buffersink_get_frame_flags(filter.ctx(), out.frame(), 0);
                if (ret < 0)
                    break;

                if (!out.frame()->pkt_duration)
                    out.frame()->pkt_duration = d->sourceFrame.frame()->pkt_duration;
                frame.setTimeBase(av_buffersink_get_time_base(filter.ctx()));
                d->outputFrames.push_back(out);
            }
        }
    }

    if (d->outputFrames.isEmpty()) {
        frame = d->sourceFrame;
        d->sourceFrame = {};
        return ret;
    }

    frame = d->outputFrames.takeFirst();
    return 0;
}

QT_END_NAMESPACE
