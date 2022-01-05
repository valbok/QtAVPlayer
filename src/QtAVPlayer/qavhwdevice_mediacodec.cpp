/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_mediacodec_p.h"
#include "qavvideobuffer_cpu_p.h"

extern "C" {
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

QAVHWDevice_MediaCodec::QAVHWDevice_MediaCodec(QObject *parent)
    : QObject(parent)
{
}

AVPixelFormat QAVHWDevice_MediaCodec::format() const
{
    return AV_PIX_FMT_MEDIACODEC;
}

AVHWDeviceType QAVHWDevice_MediaCodec::type() const
{
    return AV_HWDEVICE_TYPE_MEDIACODEC;
}

QAVVideoBuffer *QAVHWDevice_MediaCodec::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_CPU(frame);
}

QT_END_NAMESPACE
