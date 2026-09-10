/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavhwdevice_vulkan_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <QDebug>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
}

QT_BEGIN_NAMESPACE

AVPixelFormat QAVHWDevice_Vulkan::format() const
{
    return AV_PIX_FMT_VULKAN;
}

AVHWDeviceType QAVHWDevice_Vulkan::type() const
{
    return AV_HWDEVICE_TYPE_VULKAN;
}

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

QT_END_NAMESPACE
