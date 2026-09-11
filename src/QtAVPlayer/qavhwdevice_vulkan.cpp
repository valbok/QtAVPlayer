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

AVPixelFormat QAVHWDevice_Vulkan::format() const
{
    return AV_PIX_FMT_VULKAN;
}

AVHWDeviceType QAVHWDevice_Vulkan::type() const
{
    return AV_HWDEVICE_TYPE_VULKAN;
}

#if defined(QT_AVPLAYER_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

class VideoBuffer_Vulkan: public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_Vulkan(const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
    {
    }

    ~VideoBuffer_Vulkan()
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

        AVFrame *av_frame = frame().frame();
        AVVkFrame* vk_frame = (AVVkFrame*)av_frame->data[0];
        VkImage singleVkImage = vk_frame->img[0];

        QList<quint64> textures = {
            reinterpret_cast<quint64>(singleVkImage),
            reinterpret_cast<quint64>(singleVkImage)
        };
        return QVariant::fromValue(textures);
    }
};

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_Vulkan(frame);
}

#else // #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

#endif
QT_END_NAMESPACE
