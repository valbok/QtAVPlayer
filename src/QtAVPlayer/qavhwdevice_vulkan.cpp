/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavhwdevice_vulkan_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <QDebug>

#if defined(QT_AVPLAYER_MULTIMEDIA) && defined(QT_AVPLAYER_VULKAN) && QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <private/qrhi_p.h>
#include <QtMultimedia/private/qvideoframetexturefromsource_p.h>
#endif

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
#if defined(QT_AVPLAYER_MULTIMEDIA) && defined(QT_AVPLAYER_VULKAN) && QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <libavutil/hwcontext_vulkan.h>
#endif
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

#if defined(QT_AVPLAYER_MULTIMEDIA) && defined(QT_AVPLAYER_VULKAN) && QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)

class QVideoFrameTexturesFromRhiTextureArray : public QVideoFrameTextures
{
public:
    QVideoFrameTexturesFromRhiTextureArray(QVideoTextureHelper::RhiTextureArray &&rhiTextures = {})
        : m_rhiTextures(std::move(rhiTextures))
    {
    }

    QRhiTexture *texture(uint plane) const override
    {
        return plane < m_rhiTextures.size() ? m_rhiTextures[plane].get() : nullptr;
    }

    QVideoTextureHelper::RhiTextureArray &textureArray() { return m_rhiTextures; }

private:
    QVideoTextureHelper::RhiTextureArray m_rhiTextures;
};

#define QAV_VK_DEVICE_FUNCTIONS(F) \
    F(CreateCommandPool) \
    F(DestroyCommandPool) \
    F(ResetCommandPool) \
    F(AllocateCommandBuffers) \
    F(BeginCommandBuffer) \
    F(EndCommandBuffer) \
    F(CmdPipelineBarrier) \
    F(CmdCopyImage) \
    F(CreateFence) \
    F(DestroyFence) \
    F(ResetFences) \
    F(WaitForFences) \
    F(QueueSubmit)

struct VulkanFunctions
{
#define QAV_VK_DECLARE(name) PFN_vk##name name = nullptr;
    QAV_VK_DEVICE_FUNCTIONS(QAV_VK_DECLARE)
#undef QAV_VK_DECLARE

    bool load(const AVVulkanDeviceContext *hwctx)
    {
        if (!hwctx->get_proc_addr)
            return false;
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        auto getDeviceProcAddr = (PFN_vkGetDeviceProcAddr)hwctx->get_proc_addr(hwctx->inst, "vkGetDeviceProcAddr");
        if (!getDeviceProcAddr)
            return false;
#define QAV_VK_LOAD(n) \
        n = (PFN_vk##n)getDeviceProcAddr(hwctx->act_dev, "vk" #n); \
        if (!n) \
            return false;
        QAV_VK_DEVICE_FUNCTIONS(QAV_VK_LOAD)
#undef QAV_VK_LOAD
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
        return true;
    }
};

class VulkanPlaneCopy
{
public:
    ~VulkanPlaneCopy()
    {
        release();
    }

    bool copy(QRhi *rhi, const AVFrame *frame, const QVideoTextureHelper::RhiTextureArray &textures)
    {
        auto framesCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
        if (framesCtx->sw_format != AV_PIX_FMT_NV12) {
            qWarning() << "Format is not supported:" << framesCtx->sw_format;
            return false;
        }
        auto vkFrames = reinterpret_cast<AVVulkanFramesContext *>(framesCtx->hwctx);
        auto nh = static_cast<const QRhiVulkanNativeHandles *>(rhi->nativeHandles());
        if (!nh || !nh->gfxQueue) {
            qWarning() << "No QRhiVulkanNativeHandles";
            return false;
        }

        if (!init(framesCtx, nh->gfxQueueFamilyIdx))
            return false;

        auto vkFrame = reinterpret_cast<AVVkFrame *>(frame->data[0]);
        int nbImages = 1;
        while (nbImages < AV_NUM_DATA_POINTERS && vkFrame->img[nbImages])
            ++nbImages;
        const int nbPlanes = 2;

        // Wait for the previous copy since the command buffer is reused
        if (m_submitted) {
            m_fn.WaitForFences(m_dev, 1, &m_fence, VK_TRUE, UINT64_MAX);
            m_submitted = false;
        }
        m_fn.ResetFences(m_dev, 1, &m_fence);
        m_fn.ResetCommandPool(m_dev, m_pool, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (m_fn.BeginCommandBuffer(m_cmd, &beginInfo) != VK_SUCCESS)
            return false;

        // The frame properties must not be changed by other threads until the submission
        vkFrames->lock_frame(framesCtx, vkFrame);

        VkImageMemoryBarrier barriers[AV_NUM_DATA_POINTERS * 2] = {};
        int nbBarriers = 0;
        for (int i = 0; i < nbImages; ++i) {
            auto &b = barriers[nbBarriers++];
            b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.srcAccessMask = vkFrame->access[i];
            b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            b.oldLayout = vkFrame->layout[i];
            b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = vkFrame->img[i];
            b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        }
        for (int p = 0; p < nbPlanes; ++p) {
            auto &b = barriers[nbBarriers++];
            b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.srcAccessMask = 0;
            b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = VkImage(textures[p]->nativeTexture().object);
            b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        }
        m_fn.CmdPipelineBarrier(m_cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0, 0, nullptr, 0, nullptr, nbBarriers, barriers);

        for (int p = 0; p < nbPlanes; ++p) {
            const QSize extent = p == 0 ? QSize{frame->width, frame->height}: QSize{frame->width / 2, frame->height / 2};
            VkImageCopy region = {};
            region.srcSubresource.aspectMask = VkImageAspectFlags(VK_IMAGE_ASPECT_PLANE_0_BIT << p);
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.layerCount = 1;
            region.extent = { uint32_t(extent.width()), uint32_t(extent.height()), 1 };
            m_fn.CmdCopyImage(m_cmd,
                              vkFrame->img[0], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VkImage(textures[p]->nativeTexture().object), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              1, &region);
        }

        nbBarriers = 0;
        for (int p = 0; p < nbPlanes; ++p) {
            auto &b = barriers[nbBarriers++];
            b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = VkImage(textures[p]->nativeTexture().object);
            b.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        }
        m_fn.CmdPipelineBarrier(m_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0, 0, nullptr, 0, nullptr, nbBarriers, barriers);

        bool ok = m_fn.EndCommandBuffer(m_cmd) == VK_SUCCESS;
        if (ok) {
            // Wait for the decoder and let the decoder wait for the copy
            VkSemaphore semaphores[AV_NUM_DATA_POINTERS];
            uint64_t waitValues[AV_NUM_DATA_POINTERS];
            uint64_t signalValues[AV_NUM_DATA_POINTERS];
            VkPipelineStageFlags waitStages[AV_NUM_DATA_POINTERS];
            for (int i = 0; i < nbImages; ++i) {
                semaphores[i] = vkFrame->sem[i];
                waitValues[i] = vkFrame->sem_value[i];
                signalValues[i] = vkFrame->sem_value[i] + 1;
                waitStages[i] = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }

            VkTimelineSemaphoreSubmitInfo timelineInfo = {};
            timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
            timelineInfo.waitSemaphoreValueCount = nbImages;
            timelineInfo.pWaitSemaphoreValues = waitValues;
            timelineInfo.signalSemaphoreValueCount = nbImages;
            timelineInfo.pSignalSemaphoreValues = signalValues;

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = &timelineInfo;
            submitInfo.waitSemaphoreCount = nbImages;
            submitInfo.pWaitSemaphores = semaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &m_cmd;
            submitInfo.signalSemaphoreCount = nbImages;
            submitInfo.pSignalSemaphores = semaphores;

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(60, 29, 100)
            auto *deviceCtx = framesCtx->device_ctx;
            auto *hwctx = reinterpret_cast<AVVulkanDeviceContext *>(deviceCtx->hwctx);
            hwctx->lock_queue(deviceCtx, nh->gfxQueueFamilyIdx, nh->gfxQueueIdx);
#endif
            VkResult res = m_fn.QueueSubmit(nh->gfxQueue, 1, &submitInfo, m_fence);
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(60, 29, 100)
            hwctx->unlock_queue(deviceCtx, nh->gfxQueueFamilyIdx, nh->gfxQueueIdx);
#endif
            ok = res == VK_SUCCESS;
            if (ok) {
                m_submitted = true;
                for (int i = 0; i < nbImages; ++i) {
                    vkFrame->sem_value[i] = signalValues[i];
                    vkFrame->layout[i] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    vkFrame->access[i] = VK_ACCESS_TRANSFER_READ_BIT;
                }
            } else {
                qWarning() << "Could not submit Vulkan copy:" << res;
            }
        }
        vkFrames->unlock_frame(framesCtx, vkFrame);
        return ok;
    }

private:
    bool init(AVHWFramesContext *framesCtx, uint32_t queueFamilyIndex)
    {
        if (m_device)
            return true;
        auto hwctx = reinterpret_cast<AVVulkanDeviceContext *>(framesCtx->device_ctx->hwctx);
        if (!m_fn.load(hwctx)) {
            qWarning() << "Could not load Vulkan device functions";
            return false;
        }
        // Keep the device alive while the resources are in use
        m_device = av_buffer_ref(framesCtx->device_ref);
        m_dev = hwctx->act_dev;
        m_alloc = hwctx->alloc;

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        if (m_fn.CreateCommandPool(m_dev, &poolInfo, m_alloc, &m_pool) != VK_SUCCESS) {
            qWarning() << "Could not create Vulkan command pool";
            release();
            return false;
        }

        VkCommandBufferAllocateInfo cmdInfo = {};
        cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdInfo.commandPool = m_pool;
        cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdInfo.commandBufferCount = 1;
        if (m_fn.AllocateCommandBuffers(m_dev, &cmdInfo, &m_cmd) != VK_SUCCESS) {
            qWarning() << "Could not allocate Vulkan command buffer";
            release();
            return false;
        }

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (m_fn.CreateFence(m_dev, &fenceInfo, m_alloc, &m_fence) != VK_SUCCESS) {
            qWarning() << "Could not create Vulkan fence";
            release();
            return false;
        }
        return true;
    }

    void release()
    {
        if (!m_device)
            return;
        if (m_submitted)
            m_fn.WaitForFences(m_dev, 1, &m_fence, VK_TRUE, UINT64_MAX);
        if (m_fence)
            m_fn.DestroyFence(m_dev, m_fence, m_alloc);
        if (m_pool)
            m_fn.DestroyCommandPool(m_dev, m_pool, m_alloc);
        m_fence = VK_NULL_HANDLE;
        m_pool = VK_NULL_HANDLE;
        m_cmd = VK_NULL_HANDLE;
        m_submitted = false;
        av_buffer_unref(&m_device);
    }

    AVBufferRef *m_device = nullptr;
    VulkanFunctions m_fn;
    VkDevice m_dev = VK_NULL_HANDLE;
    const VkAllocationCallbacks *m_alloc = nullptr;
    VkCommandPool m_pool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmd = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;
    bool m_submitted = false;
};

class VideoBuffer_Vulkan: public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_Vulkan(const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
    {
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::VulkanTextureHandle;
    }

    QVariant handle(QRhi *) const override
    {
        return {};
    }

    QVideoFrameTextures *mapTextures(QRhi &rhi) override
    {
        const auto *texDesc = QVideoTextureHelper::textureDescription(QVideoFrameFormat::Format_NV12);
        if (rhi.backend() != QRhi::Vulkan
            || frame().format() != AV_PIX_FMT_VULKAN
            || !texDesc)
            return nullptr;

        QVideoTextureHelper::RhiTextureArray textures;
        for (quint8 plane = 0; plane < texDesc->nplanes; ++plane) {
            QSize planeSize = texDesc->rhiPlaneSize(frame().size(), plane, &rhi);
            auto texture = std::unique_ptr<QRhiTexture>(
                rhi.newTexture(texDesc->rhiTextureFormat(plane, &rhi), planeSize, 1,
                               QRhiTexture::UsedAsTransferDestination));
                return nullptr;
            textures[plane] = std::move(texture);
        }

        if (!m_copy.copy(&rhi, frame().frame(), textures))
            return nullptr;

        return new QVideoFrameTexturesFromRhiTextureArray(std::move(textures));
    }

    VulkanPlaneCopy m_copy;
};

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_Vulkan(frame);
}

#else // #if defined(QT_AVPLAYER_MULTIMEDIA) && defined(QT_AVPLAYER_VULKAN) && QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

#endif
QT_END_NAMESPACE
