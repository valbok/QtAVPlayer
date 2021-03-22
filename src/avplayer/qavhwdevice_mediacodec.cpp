/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_mediacodec_p.h"
#include "qavvideocodec_p.h"
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

QAVVideoFrame::MapData QAVHWDevice_MediaCodec::map(const QAVVideoFrame &frame) const
{
    return QAVVideoBuffer_CPU(frame).map();
}

QAVVideoFrame::HandleType QAVHWDevice_MediaCodec::handleType() const
{
    return QAVVideoFrame::NoHandle;
}

QVariant QAVHWDevice_MediaCodec::handle(const QAVVideoFrame &frame) const
{
    return QAVVideoBuffer_CPU(frame).handle();
}

QT_END_NAMESPACE
