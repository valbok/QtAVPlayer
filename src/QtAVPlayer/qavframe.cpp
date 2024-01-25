/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavframe.h"
#include "qavstream.h"
#include "qavframe_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

QAVFrame::QAVFrame()
    : QAVFrame(*new QAVFramePrivate)
{
}

QAVFrame::QAVFrame(const QAVFrame &other)
    : QAVFrame()
{
    *this = other;
}

QAVFrame::QAVFrame(QAVFramePrivate &d)
    : QAVStreamFrame(d)
{
    d.frame = av_frame_alloc();
}

QAVFrame &QAVFrame::operator=(const QAVFrame &other)
{
    Q_D(QAVFrame);
    QAVStreamFrame::operator=(other);

    auto other_priv = static_cast<QAVFramePrivate *>(other.d_ptr.get());
    int64_t pts = d->frame->pts;
    av_frame_unref(d->frame);
    av_frame_ref(d->frame, other_priv->frame);

    if (d->frame->pts < 0)
        d->frame->pts = pts;

    d->frameRate = other_priv->frameRate;
    d->timeBase = other_priv->timeBase;
    d->filterName = other_priv->filterName;
    return *this;
}

QAVFrame::operator bool() const
{
    Q_D(const QAVFrame);
    return QAVStreamFrame::operator bool() && d->frame && (d->frame->data[0] || d->frame->data[1] || d->frame->data[2] || d->frame->data[3]);
}

QAVFrame::~QAVFrame()
{
    Q_D(QAVFrame);
    av_frame_free(&d->frame);
}

AVFrame *QAVFrame::frame() const
{
    Q_D(const QAVFrame);
    return d->frame;
}

void QAVFrame::setFrameRate(const AVRational &value)
{
    Q_D(QAVFrame);
    d->frameRate = value;
}

void QAVFrame::setTimeBase(const AVRational &value)
{
    Q_D(QAVFrame);
    d->timeBase = value;
}

QString QAVFrame::filterName() const
{
    return d_func()->filterName;
}

void QAVFrame::setFilterName(const QString &name)
{
    Q_D(QAVFrame);
    d->filterName = name;
}

double QAVFramePrivate::pts() const
{
    if (!frame || !stream)
        return NAN;

    AVRational tb = timeBase.num && timeBase.den ? timeBase : stream.stream()->time_base;
    return frame->pts == AV_NOPTS_VALUE ? NAN : frame->pts * av_q2d(tb);
}

double QAVFramePrivate::duration() const
{
    if (!frame || !stream)
        return 0.0;

    return frameRate.den && frameRate.num
           ? av_q2d(AVRational{frameRate.den, frameRate.num})
           :
#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 30, 0)
             frame->pkt_duration
#else
             frame->duration
#endif
             * av_q2d(stream.stream()->time_base);
}

QT_END_NAMESPACE
