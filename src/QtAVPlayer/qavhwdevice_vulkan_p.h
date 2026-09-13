/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVHWDEVICE_VULKAN_P_H
#define QAVHWDEVICE_VULKAN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qavhwdevice_p.h"
#include <memory>

QT_BEGIN_NAMESPACE

class QVulkanInstance;
class Q_AVPLAYER_EXPORT QAVHWDevice_Vulkan : public QAVHWDevice
{
public:
    QAVHWDevice_Vulkan() = default;
    ~QAVHWDevice_Vulkan() = default;

    AVPixelFormat format() const override;
    AVHWDeviceType type() const override;
    QAVVideoBuffer *videoBuffer(const QAVVideoFrame &frame) const override;
    int createDeviceContext(AVBufferRef **ctx, AVDictionary *opts) override;

    // All video codecs share one Vulkan device context (AV_HWDEVICE_TYPE_VULKAN) per process.
    // To render the decoded frames without copying them to CPU memory,
    // Qt must render using the same device, see setupInstance() and renderDevice().
    // Returns a new reference which must be released with av_buffer_unref(),
    // or nullptr if the device could not be created.
    static AVBufferRef *deviceContext();

    // Vulkan objects of the shared device context to be used for rendering,
    // e.g. by QQuickGraphicsDevice::fromDeviceObjects()
    struct RenderDevice
    {
        void *physicalDevice = nullptr; // VkPhysicalDevice
        void *device = nullptr;         // VkDevice
        int queueFamilyIndex = -1;      // Queue family with graphics support
        int queueIndex = 0;             // Queue reserved for rendering
    };
    static RenderDevice renderDevice();

    // Makes the instance adopt VkInstance of the shared device context.
    // Must be called before QVulkanInstance::create().
    static bool setupInstance(QVulkanInstance *instance);

private:
    Q_DISABLE_COPY(QAVHWDevice_Vulkan)
};

QT_END_NAMESPACE

#endif
