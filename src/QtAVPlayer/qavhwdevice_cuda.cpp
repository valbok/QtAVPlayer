/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavhwdevice_cuda_p.h"
#include "qavvideobuffer_gpu_p.h"

extern "C" {
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

AVPixelFormat QAVHWDevice_CUDA::format() const
{
    return AV_PIX_FMT_CUDA;
}

AVHWDeviceType QAVHWDevice_CUDA::type() const
{
    return AV_HWDEVICE_TYPE_CUDA;
}

QAVVideoBuffer *QAVHWDevice_CUDA::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

QT_END_NAMESPACE
