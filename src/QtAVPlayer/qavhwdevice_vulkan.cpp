/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavhwdevice_vulkan_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <QtGui/qtguiglobal.h>
#include <QDebug>
#include <QList>
#include <QMutex>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
}

// Timeline semaphores and lock_frame() are required
#if defined(QT_AVPLAYER_VULKAN) && LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 11, 100)
    #define QT_AVPLAYER_VULKAN_DEVICE
    extern "C" {
    #include <libavutil/hwcontext_vulkan.h>
    }
    #if QT_CONFIG(vulkan)
        #include <QVulkanInstance>
    #endif
    #if defined(QT_AVPLAYER_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 4, 0) && QT_CONFIG(vulkan)
        #define QT_AVPLAYER_VULKAN_RHI
        #include <private/qrhi_p.h>
        #if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
            #include <private/qrhivulkan_p.h>
        #endif
    #endif
#endif

QT_BEGIN_NAMESPACE

AVPixelFormat QAVHWDevice_Vulkan::format() const
{
    return AV_PIX_FMT_VULKAN;
}

AVHWDeviceType QAVHWDevice_Vulkan::type() const
{
    return AV_HWDEVICE_TYPE_VULKAN;
}

#if defined(QT_AVPLAYER_VULKAN_DEVICE)

static QMutex s_deviceMutex;
// The device context lives until the process exits,
// since the renderer might still use the device while ffmpeg does not need it anymore.
static AVBufferRef *s_deviceContext = nullptr;

// Reserves the last queue of the graphics queue family for rendering:
// ffmpeg rotates the queues of a family for its submissions,
// and VkQueue must not be used from multiple threads simultaneously.
static QAVHWDevice_Vulkan::RenderDevice reserveRenderQueue(AVHWDeviceContext *ctx)
{
    auto hwctx = reinterpret_cast<AVVulkanDeviceContext *>(ctx->hwctx);
    QAVHWDevice_Vulkan::RenderDevice dev;
    dev.physicalDevice = hwctx->phys_dev;
    dev.device = hwctx->act_dev;
    int nbQueues = 0;
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(59, 34, 100)
    for (int i = 0; i < hwctx->nb_qf; ++i) {
        if (hwctx->qf[i].flags & VK_QUEUE_GRAPHICS_BIT) {
            dev.queueFamilyIndex = hwctx->qf[i].idx;
            nbQueues = hwctx->qf[i].num;
            if (nbQueues > 1)
                hwctx->qf[i].num = nbQueues - 1;
            break;
        }
    }
#endif
#if LIBAVUTIL_VERSION_MAJOR < 60
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    if (dev.queueFamilyIndex < 0) {
        dev.queueFamilyIndex = hwctx->queue_family_index;
        nbQueues = hwctx->nb_graphics_queues;
    }
    if (nbQueues > 1) {
        // The same queue family could be used for different purposes
        int *families[] = { &hwctx->queue_family_index, &hwctx->queue_family_tx_index, &hwctx->queue_family_comp_index,
                            &hwctx->queue_family_encode_index, &hwctx->queue_family_decode_index };
        int *counts[] = { &hwctx->nb_graphics_queues, &hwctx->nb_tx_queues, &hwctx->nb_comp_queues,
                          &hwctx->nb_encode_queues, &hwctx->nb_decode_queues };
        for (size_t i = 0; i < sizeof(families) / sizeof(families[0]); ++i) {
            if (*families[i] == dev.queueFamilyIndex && *counts[i] == nbQueues)
                *counts[i] = nbQueues - 1;
        }
    }
QT_WARNING_POP
#endif
    dev.queueIndex = nbQueues > 1 ? nbQueues - 1 : 0;
    if (nbQueues <= 1)
        qWarning() << "Vulkan graphics queue family" << dev.queueFamilyIndex << "has only one queue, which is shared with ffmpeg";
    return dev;
}

static QAVHWDevice_Vulkan::RenderDevice s_renderDevice;

AVBufferRef *QAVHWDevice_Vulkan::deviceContext()
{
    QMutexLocker locker(&s_deviceMutex);
    if (!s_deviceContext) {
        AVDictionary *opts = nullptr;
        // Extensions required to present to a window. Unsupported ones are skipped by ffmpeg.
        av_dict_set(&opts, "instance_extensions",
                    "VK_KHR_surface"
                    "+VK_KHR_xcb_surface"
                    "+VK_KHR_xlib_surface"
                    "+VK_KHR_wayland_surface"
                    "+VK_KHR_win32_surface"
                    "+VK_KHR_android_surface"
                    "+VK_EXT_metal_surface"
                    "+VK_KHR_get_physical_device_properties2", 0);
        av_dict_set(&opts, "device_extensions", "VK_KHR_swapchain", 0);
        int ret = av_hwdevice_ctx_create(&s_deviceContext, AV_HWDEVICE_TYPE_VULKAN, nullptr, opts, 0);
        av_dict_free(&opts);
        if (ret < 0) {
            qWarning() << "Could not create Vulkan device context:" << ret;
            av_buffer_unref(&s_deviceContext);
            return nullptr;
        }
        s_renderDevice = reserveRenderQueue(reinterpret_cast<AVHWDeviceContext *>(s_deviceContext->data));
    }

    return av_buffer_ref(s_deviceContext);
}

QAVHWDevice_Vulkan::RenderDevice QAVHWDevice_Vulkan::renderDevice()
{
    auto ctx = deviceContext();
    if (!ctx)
        return {};
    av_buffer_unref(&ctx);
    QMutexLocker locker(&s_deviceMutex);
    return s_renderDevice;
}

bool QAVHWDevice_Vulkan::setupInstance(QVulkanInstance *instance)
{
#if QT_CONFIG(vulkan)
    if (!instance)
        return false;
    auto ctx = deviceContext();
    if (!ctx)
        return false;
    auto hwctx = reinterpret_cast<AVVulkanDeviceContext *>(reinterpret_cast<AVHWDeviceContext *>(ctx->data)->hwctx);
    QByteArrayList extensions;
    for (int i = 0; i < hwctx->nb_enabled_inst_extensions; ++i)
        extensions.append(hwctx->enabled_inst_extensions[i]);
    instance->setVkInstance(hwctx->inst);
    instance->setExtensions(extensions);
    instance->setApiVersion(QVersionNumber(1, 3));
    av_buffer_unref(&ctx);
    return true;
#else
    Q_UNUSED(instance);
    return false;
#endif
}

int QAVHWDevice_Vulkan::createDeviceContext(AVBufferRef **ctx, AVDictionary */*opts*/)
{
    *ctx = deviceContext();
    return *ctx ? 0 : AVERROR(ENOSYS);
}

#if defined(QT_AVPLAYER_VULKAN_RHI)

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

struct PlaneFormat
{
    QRhiTexture::Format format = QRhiTexture::UnknownFormat;
    int divX = 1;
    int divY = 1;
};

// Formats of the planes as expected by QVideoFrameFormat of the same pixel format
static int planeFormats(AVPixelFormat swFormat, PlaneFormat planes[AV_NUM_DATA_POINTERS])
{
    switch (swFormat) {
    case AV_PIX_FMT_NV12:
        planes[0] = { QRhiTexture::R8, 1, 1 };
        planes[1] = { QRhiTexture::RG8, 2, 2 };
        return 2;
    case AV_PIX_FMT_P010:
    case AV_PIX_FMT_P016:
        planes[0] = { QRhiTexture::R16, 1, 1 };
        planes[1] = { QRhiTexture::RG16, 2, 2 };
        return 2;
    default:
        return 0;
    }
}

static QSize planeSize(const QSize &size, const PlaneFormat &plane)
{
    return { (size.width() + plane.divX - 1) / plane.divX, (size.height() + plane.divY - 1) / plane.divY };
}

// Extracts the planes of AVVkFrame to separate images which can be sampled by the rhi.
// Vulkan video decoders produce one multiplane image with all planes,
// while Qt renders one single plane texture per plane.
// The copy is done by the queue used by the rhi to keep the submissions ordered.
class VulkanPlaneCopy
{
public:
    VulkanPlaneCopy() = default;
    ~VulkanPlaneCopy()
    {
        release();
    }

    bool copy(QRhi *rhi, const AVFrame *frame, const QList<QRhiTexture *> &textures, const PlaneFormat *planes)
    {
        auto framesCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
        auto vkFrames = reinterpret_cast<AVVulkanFramesContext *>(framesCtx->hwctx);
        auto deviceCtx = framesCtx->device_ctx;
        auto hwctx = reinterpret_cast<AVVulkanDeviceContext *>(deviceCtx->hwctx);
        auto nh = static_cast<const QRhiVulkanNativeHandles *>(rhi->nativeHandles());
        if (!nh || !nh->gfxQueue) {
            qWarning() << "No QRhiVulkanNativeHandles";
            return false;
        }

        if (nh->dev != hwctx->act_dev) {
            static bool warned = false;
            if (!warned) {
                qWarning() << "QRhi does not use the Vulkan device of the decoder, "
                              "see QAVHWDevice_Vulkan::setupInstance() and QAVHWDevice_Vulkan::renderDevice()";
                warned = true;
            }
            return false;
        }

        if (!init(framesCtx, nh->gfxQueueFamilyIdx))
            return false;

        auto vkFrame = reinterpret_cast<AVVkFrame *>(frame->data[0]);
        int nbImages = 0;
        while (nbImages < AV_NUM_DATA_POINTERS && vkFrame->img[nbImages])
            ++nbImages;
        const int nbPlanes = textures.size();
        const bool multiplane = nbImages == 1 && nbPlanes > 1;
        if (!nbImages || (!multiplane && nbImages < nbPlanes)) {
            qWarning() << "Unexpected number of Vulkan images:" << nbImages << "for planes:" << nbPlanes;
            return false;
        }

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

        const QSize size(frame->width, frame->height);
        for (int p = 0; p < nbPlanes; ++p) {
            const QSize extent = planeSize(size, planes[p]);
            VkImageCopy region = {};
            region.srcSubresource.aspectMask = multiplane ? VkImageAspectFlags(VK_IMAGE_ASPECT_PLANE_0_BIT << p) : VkImageAspectFlags(VK_IMAGE_ASPECT_COLOR_BIT);
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.layerCount = 1;
            region.extent = { uint32_t(extent.width()), uint32_t(extent.height()), 1 };
            m_fn.CmdCopyImage(m_cmd,
                              vkFrame->img[multiplane ? 0 : p], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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

        if (ok) {
            for (auto *texture : textures)
                texture->setNativeLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
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

#endif // #if defined(QT_AVPLAYER_VULKAN_RHI)

class VideoBuffer_Vulkan : public QAVVideoBuffer_GPU
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

    QVariant handle(QRhi */*rhi*/) const override
    {
        if (!frame() || frame().format() != AV_PIX_FMT_VULKAN)
            return {};

        auto vkFrame = reinterpret_cast<AVVkFrame *>(frame().frame()->data[0]);
        if (!vkFrame)
            return {};

        QList<quint64> images;
        for (int i = 0; i < AV_NUM_DATA_POINTERS && vkFrame->img[i]; ++i)
            images.append(quint64(vkFrame->img[i]));
        return QVariant::fromValue(images);
    }

#if defined(QT_AVPLAYER_VULKAN_RHI)
    QList<QRhiTexture *> mapRhiTextures(QRhi *rhi) override
    {
        if (!rhi || rhi->backend() != QRhi::Vulkan || !frame() || frame().format() != AV_PIX_FMT_VULKAN)
            return {};

        auto av_frame = frame().frame();
        if (!av_frame->hw_frames_ctx || !av_frame->data[0])
            return {};

        auto framesCtx = reinterpret_cast<AVHWFramesContext *>(av_frame->hw_frames_ctx->data);
        PlaneFormat planes[AV_NUM_DATA_POINTERS];
        const int nbPlanes = planeFormats(framesCtx->sw_format, planes);
        if (!nbPlanes) {
            qWarning() << "Vulkan textures are not supported for" << av_get_pix_fmt_name(framesCtx->sw_format);
            return {};
        }

        const QSize size(av_frame->width, av_frame->height);
        QList<QRhiTexture *> textures;
        for (int p = 0; p < nbPlanes; ++p) {
            auto texture = rhi->newTexture(planes[p].format, planeSize(size, planes[p]), 1);
            if (!texture || !texture->create()) {
                qWarning() << "Could not create rhi texture for plane" << p;
                delete texture;
                qDeleteAll(textures);
                return {};
            }
            textures.append(texture);
        }

        if (!m_copy.copy(rhi, av_frame, textures, planes)) {
            qDeleteAll(textures);
            return {};
        }

        return textures;
    }

private:
    VulkanPlaneCopy m_copy;
#endif // #if defined(QT_AVPLAYER_VULKAN_RHI)
};

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_Vulkan(frame);
}

#else // #if defined(QT_AVPLAYER_VULKAN_DEVICE)

AVBufferRef *QAVHWDevice_Vulkan::deviceContext()
{
    return nullptr;
}

QAVHWDevice_Vulkan::RenderDevice QAVHWDevice_Vulkan::renderDevice()
{
    return {};
}

bool QAVHWDevice_Vulkan::setupInstance(QVulkanInstance *)
{
    return false;
}

int QAVHWDevice_Vulkan::createDeviceContext(AVBufferRef **ctx, AVDictionary *opts)
{
    return QAVHWDevice::createDeviceContext(ctx, opts);
}

QAVVideoBuffer *QAVHWDevice_Vulkan::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

#endif // #if defined(QT_AVPLAYER_VULKAN_DEVICE)

QT_END_NAMESPACE
