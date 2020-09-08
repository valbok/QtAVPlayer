/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideoframe_p.h"
#include "qavplanarvideobuffer_cpu_p.h"
#include "qavframe_p_p.h"
#include "qavvideocodec_p.h"
#include "qavhwdevice_p.h"
#include <QVideoFrame>
#include <QDebug>

QT_BEGIN_NAMESPACE

QAVVideoFrame::QAVVideoFrame(QObject *parent)
    : QAVFrame(parent)
{
}

const QAVVideoCodec *QAVVideoFrame::codec() const
{
    return reinterpret_cast<const QAVVideoCodec *>(d_func()->codec);
}

QAVVideoFrame::QAVVideoFrame(const QAVFrame &other, QObject *parent)
    : QAVVideoFrame(parent)
{
    operator=(other);
}

QAVVideoFrame &QAVVideoFrame::operator=(const QAVFrame &other)
{
    QAVFrame::operator=(other);
    return *this;
}

QVideoFrame::PixelFormat QAVVideoFrame::pixelFormat(AVPixelFormat from)
{
    switch (from) {
    case AV_PIX_FMT_YUV420P:
        return QVideoFrame::Format_YUV420P;
    case AV_PIX_FMT_NV12:
        return QVideoFrame::Format_NV12;
    case AV_PIX_FMT_BGRA:
        return QVideoFrame::Format_BGRA32;
    case AV_PIX_FMT_ARGB:
        return QVideoFrame::Format_ARGB32;
    default:
        return QVideoFrame::Format_Invalid;
    }
}

AVPixelFormat QAVVideoFrame::pixelFormat(QVideoFrame::PixelFormat from)
{
    switch (from) {
    case QVideoFrame::Format_YUV420P:
        return AV_PIX_FMT_YUV420P;
    case QVideoFrame::Format_NV12:
        return AV_PIX_FMT_NV12;
    default:
        return AV_PIX_FMT_NONE;
    }
}

QSize QAVVideoFrame::size() const
{
    Q_D(const QAVFrame);
    return {d->frame->width, d->frame->height};
}

QAVVideoFrame::operator QVideoFrame() const
{
    Q_D(const QAVFrame);
    if (!codec())
        return {};

    if (codec()->device())
        return codec()->device()->decode(*this);

    auto format = pixelFormat(AVPixelFormat(d->frame->format));
    return {new QAVPlanarVideoBuffer_CPU(*this), size(), format};
}

QT_END_NAMESPACE
