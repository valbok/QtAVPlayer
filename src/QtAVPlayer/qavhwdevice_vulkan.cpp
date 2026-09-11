/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavhwdevice_vulkan_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <QDebug>

#if defined(QT_AVPLAYER_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qrhi_p.h>
#endif

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext_vulkan.h>
}

QT_BEGIN_NAMESPACE

void QAVHWDevice_Vulkan::init(AVCodecContext *avctx)
{
    QAVHWDevice::init(avctx);
    if (!avctx || !avctx->hw_device_ctx)
        return;

    if (avctx->hw_frames_ctx)
        av_buffer_unref(&avctx->hw_frames_ctx);

    int ret = avcodec_get_hw_frames_parameters(avctx,
                                               avctx->hw_device_ctx,
                                               AV_PIX_FMT_VULKAN,
                                               &avctx->hw_frames_ctx);
    if (ret < 0) {
        qWarning() << "Failed to allocate HW frames context:" << ret;
        return;
    }

    auto frames_ctx = reinterpret_cast<AVHWFramesContext *>(avctx->hw_frames_ctx->data);
    auto hwctx = reinterpret_cast<AVVulkanFramesContext *>(frames_ctx->hwctx);
    hwctx->flags = static_cast<AVVkFrameFlags>(hwctx->flags | AV_VK_FRAME_FLAG_DISABLE_MULTIPLANE);

    ret = av_hwframe_ctx_init(avctx->hw_frames_ctx);
    if (ret < 0) {
        qWarning() << "Failed to initialize HW frames context:" << ret;
        av_buffer_unref(&avctx->hw_frames_ctx);
    }
}

QVariant QAVHWDevice_Vulkan::textureHandles(const AVFrame *av_frame)
{
    if (!av_frame || av_frame->format != AV_PIX_FMT_VULKAN)
        return {};

    auto vk_frame = reinterpret_cast<AVVkFrame *>(av_frame->data[0]);
    if (!vk_frame || !vk_frame->img[0])
        return {};

    auto frames_ctx = av_frame->hw_frames_ctx
        ? reinterpret_cast<AVHWFramesContext *>(av_frame->hw_frames_ctx->data)
        : nullptr;
    const auto sw_format = frames_ctx
        ? static_cast<AVPixelFormat>(frames_ctx->sw_format)
        : AV_PIX_FMT_NONE;
    const int planeCount = sw_format != AV_PIX_FMT_NONE
        ? qMax(av_pix_fmt_count_planes(sw_format), 1)
        : 1;
    const int maxImages = int(sizeof(vk_frame->img) / sizeof(vk_frame->img[0]));
    if (planeCount > maxImages) {
        qWarning() << "Too many Vulkan planes:" << planeCount << ">" << maxImages;
        return {};
    }

    QList<quint64> textures;
    textures.reserve(planeCount);
    for (int plane = 0; plane < planeCount; ++plane) {
        auto image = vk_frame->img[plane] ? vk_frame->img[plane] : vk_frame->img[0];
        if (!image)
            return {};
        textures.push_back(quint64(image));
    }
    return QVariant::fromValue(textures);
}

AVPixelFormat QAVHWDevice_Vulkan::format() const
{
    return AV_PIX_FMT_VULKAN;
}

AVHWDeviceType QAVHWDevice_Vulkan::type() const
{
    return AV_HWDEVICE_TYPE_VULKAN;
}

#if defined(QT_AVPLAYER_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

class VideoBuffer_Vulkan : public QAVVideoBuffer_GPU
{
public:
    explicit VideoBuffer_Vulkan(const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
    {
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::VulkanTextureHandle;
    }

    QVariant handle(QRhi *rhi) const override
    {
        if (!rhi || rhi->backend() != QRhi::Vulkan)
            return {};
        if (!frame() || frame().format() != AV_PIX_FMT_VULKAN)
            return {};
        auto handles = QAVHWDevice_Vulkan::textureHandles(frame().frame());
        if (handles.isNull())
            qWarning() << "No Vulkan image handles in the frame" << frame().pts();
        return handles;
    }
};

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_Vulkan(frame);
}

#else

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

#endif

QT_END_NAMESPACE
