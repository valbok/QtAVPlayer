/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavhwdevice_cuda_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <QDebug>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
}

QT_BEGIN_NAMESPACE

void QAVHWDevice_CUDA::init(AVCodecContext *avctx)
{
    AVBufferRef *frames_ref = av_hwframe_ctx_alloc(avctx->hw_device_ctx);
    AVHWFramesContext *frames_ctx = (AVHWFramesContext*)(frames_ref->data);
    frames_ctx->format    = AV_PIX_FMT_CUDA;
    frames_ctx->sw_format = AV_PIX_FMT_NV12;
    frames_ctx->width     = avctx->width;
    frames_ctx->height    = avctx->height;
    int ret = av_hwframe_ctx_init(frames_ref);
    if (ret < 0) {
        qWarning() << "Failed to initialize HW frames context:" << ret;
        av_buffer_unref(&frames_ref);
    }
    avctx->hw_frames_ctx = frames_ref;
}

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
