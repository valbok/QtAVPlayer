/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_d3d11_p.h"
#include "qavvideocodec_p.h"
#include "qavplanarvideobuffer_gpu_p.h"
#include <QVideoFrame>
#include <QAbstractVideoSurface>
#include <QDebug>

QT_BEGIN_NAMESPACE

QAVHWDevice_D3D11::QAVHWDevice_D3D11(QObject *parent)
    : QObject(parent)
{
}

AVPixelFormat QAVHWDevice_D3D11::format() const
{
    return AV_PIX_FMT_D3D11;
}

AVHWDeviceType QAVHWDevice_D3D11::type() const
{
    return AV_HWDEVICE_TYPE_D3D11VA;
}

bool QAVHWDevice_D3D11::supportsVideoSurface(QAbstractVideoSurface *surface) const
{
    if (!surface)
        return false;

    auto list = surface->supportedPixelFormats(QAbstractVideoBuffer::NoHandle);
    return list.contains(QVideoFrame::Format_NV12);
}

QVideoFrame QAVHWDevice_D3D11::decode(const QAVVideoFrame &frame) const
{
    return {new QAVPlanarVideoBuffer_GPU(frame), frame.size(), QVideoFrame::Format_NV12};
}

QT_END_NAMESPACE
