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

QAVFrame::QAVFrame(QObject *parent)
    : QAVFrame(nullptr, parent)
{
}

QAVFrame::QAVFrame(const QAVStream &stream, QObject *parent)
    : QAVFrame(*new QAVFramePrivate, parent)
{
    d_ptr->stream = stream;
}

QAVFrame::QAVFrame(const QAVFrame &other)
    : QAVFrame()
{
    *this = other;
}

QAVFrame::QAVFrame(QAVFramePrivate &d, QObject *parent)
    : QObject(parent)
    , d_ptr(&d)
{
    d_ptr->frame = av_frame_alloc();
}

QAVStream QAVFrame::stream() const
{
    return d_ptr->stream;
}

QAVFrame &QAVFrame::operator=(const QAVFrame &other)
{
    int64_t pts = d_ptr->frame->pts;
    av_frame_unref(d_ptr->frame);
    av_frame_ref(d_ptr->frame, other.d_ptr->frame);

    if (d_ptr->frame->pts < 0)
        d_ptr->frame->pts = pts;

    d_ptr->stream = other.d_ptr->stream;
    d_ptr->frameRate = other.d_ptr->frameRate;
    d_ptr->timeBase = other.d_ptr->timeBase;
    return *this;
}

QAVFrame::operator bool() const
{
    Q_D(const QAVFrame);
    return d->stream && d->frame && (d->frame->data[0] || d->frame->data[1] || d->frame->data[2] || d->frame->data[3]);
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

double QAVFrame::pts() const
{
    Q_D(const QAVFrame);
    if (!d->frame || !d->stream)
        return NAN;

    AVRational tb = d->timeBase.num && d->timeBase.den ? d->timeBase : d->stream.stream()->time_base;
    return d->frame->pts == AV_NOPTS_VALUE ? NAN : d->frame->pts * av_q2d(tb);
}

double QAVFrame::duration() const
{
    Q_D(const QAVFrame);
    if (!d->frame || !d->stream)
        return 0.0;

    return d->frameRate.den && d->frameRate.num
           ? av_q2d(AVRational{d->frameRate.den, d->frameRate.num})
           : d->frame->pkt_duration * av_q2d(d->stream.stream()->time_base);
}

QT_END_NAMESPACE
