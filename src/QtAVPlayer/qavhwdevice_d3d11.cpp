/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_d3d11_p.h"
#include "qavvideobuffer_gpu_p.h"

QT_BEGIN_NAMESPACE

QAVHWDevice_D3D11::QAVHWDevice_D3D11()
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

QAVVideoBuffer *QAVHWDevice_D3D11::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

QT_END_NAMESPACE
