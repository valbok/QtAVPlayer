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

class Q_AVPLAYER_EXPORT QAVHWDevice_Vulkan : public QAVHWDevice
{
public:
    QAVHWDevice_Vulkan() = default;
    ~QAVHWDevice_Vulkan() = default;

    void init(AVCodecContext *) override;
    static bool sameDevice(QRhi *rhi, const AVFrame *frame);
    static QVariant textureHandles(const AVFrame *frame);
    AVPixelFormat format() const override;
    AVHWDeviceType type() const override;
    QAVVideoBuffer *videoBuffer(const QAVVideoFrame &frame) const override;

private:
    Q_DISABLE_COPY(QAVHWDevice_Vulkan)
};

QT_END_NAMESPACE

#endif
