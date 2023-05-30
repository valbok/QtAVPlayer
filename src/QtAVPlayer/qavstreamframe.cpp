/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavstreamframe.h"
#include "qavstreamframe_p.h"
#include "qavframe_p.h"
#include "qavcodec_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

QAVStreamFrame::QAVStreamFrame()
    : QAVStreamFrame(*new QAVStreamFramePrivate)
{
}

QAVStreamFrame::QAVStreamFrame(const QAVStreamFrame &other)
    : QAVStreamFrame()
{
    *this = other;
}

QAVStreamFrame::QAVStreamFrame(QAVStreamFramePrivate &d)
    : d_ptr(&d)
{
}

QAVStreamFrame::~QAVStreamFrame()
{
}

QAVStream QAVStreamFrame::stream() const
{
    return d_ptr->stream;
}

void QAVStreamFrame::setStream(const QAVStream &stream)
{
    Q_D(QAVStreamFrame);
    d->stream = stream;
}

QAVStreamFrame &QAVStreamFrame::operator=(const QAVStreamFrame &other)
{
    d_ptr->stream = other.d_ptr->stream;
    return *this;
}

QAVStreamFrame::operator bool() const
{
    Q_D(const QAVStreamFrame);
    return d->stream;
}

double QAVStreamFrame::pts() const
{
    Q_D(const QAVStreamFrame);
    return d->pts();
}

double QAVStreamFrame::duration() const
{
    Q_D(const QAVStreamFrame);
    return d->duration();
}

int QAVStreamFrame::receive()
{
    Q_D(QAVStreamFrame);
    return d->stream ? d->stream.codec()->read(*this) : 0;
}

QT_END_NAMESPACE
