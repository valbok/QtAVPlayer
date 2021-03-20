/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavframe.h"
#include "qavframe_p.h"
#include "qavcodec_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

QAVFrame::QAVFrame(QObject *parent)
    : QAVFrame(nullptr, parent)
{
}

QAVFrame::QAVFrame(const QAVCodec *c, QObject *parent)
    : QAVFrame(*new QAVFramePrivate, parent)
{
    d_ptr->codec = c;
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

const QAVCodec *QAVFrame::codec() const
{
    return d_ptr->codec;
}

QAVFrame &QAVFrame::operator=(const QAVFrame &other)
{
    int64_t pts = d_ptr->frame->pts;
    av_frame_unref(d_ptr->frame);
    av_frame_ref(d_ptr->frame, other.d_ptr->frame);

    if (d_ptr->frame->pts < 0)
        d_ptr->frame->pts = pts;

    d_ptr->codec = other.d_ptr->codec;
    return *this;
}

QAVFrame::operator bool() const
{
    Q_D(const QAVFrame);
    return d->codec && d->frame && (d->frame->data[0] || d->frame->data[1] || d->frame->data[2] || d->frame->data[3]);
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

double QAVFrame::pts() const
{
    Q_D(const QAVFrame);
    if (!d->frame || !d->codec)
        return -1;

    return d->frame->pts * av_q2d(d->codec->stream()->time_base);
}

QT_END_NAMESPACE
