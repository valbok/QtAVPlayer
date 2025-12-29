/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_videotoolbox_p.h"
#include "qavvideobuffer_gpu_p.h"

#import <CoreVideo/CoreVideo.h>
#if defined(Q_OS_MACOS)
#import <IOSurface/IOSurface.h>
#else
#import <IOSurface/IOSurfaceRef.h>
#endif
#import <Metal/Metal.h>

#include <QList>
#include <QVariant>
#include <QDebug>

QT_BEGIN_NAMESPACE

class QAVHWDevice_VideoToolboxPrivate
{
public:
    id<MTLDevice> device = nullptr;
    CVPixelBufferRef pbuf = nullptr;
};

QAVHWDevice_VideoToolbox::QAVHWDevice_VideoToolbox()
    : d_ptr(new QAVHWDevice_VideoToolboxPrivate)
{
}

QAVHWDevice_VideoToolbox::~QAVHWDevice_VideoToolbox()
{
    Q_D(QAVHWDevice_VideoToolbox);
    CVPixelBufferRelease(d->pbuf);
    [d->device release];
}

AVPixelFormat QAVHWDevice_VideoToolbox::format() const
{
    return AV_PIX_FMT_VIDEOTOOLBOX;
}

AVHWDeviceType QAVHWDevice_VideoToolbox::type() const
{
    return AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
}

class VideoBuffer_MTL : public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_MTL(QAVHWDevice_VideoToolboxPrivate *hw, const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
        , m_hw(hw)
    {
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::MTLTextureHandle;
    }

    QVariant handle(QRhi */*rhi*/) const override
    {
        CVPixelBufferRelease(m_hw->pbuf);
        m_hw->pbuf = (CVPixelBufferRef)frame().frame()->data[3];
        CVPixelBufferRetain(m_hw->pbuf);
        QList<QVariant> textures = { 0, 0 };

        if (!m_hw->pbuf)
            return textures;

        if (CVPixelBufferGetDataSize(m_hw->pbuf) <= 0)
            return textures;

        auto format = CVPixelBufferGetPixelFormatType(m_hw->pbuf);
        if (format != '420v') {
            qWarning() << "420v is supported only";
            return textures;
        }

        if (!m_hw->device)
            m_hw->device = MTLCreateSystemDefaultDevice();

        IOSurfaceRef surface = CVPixelBufferGetIOSurface(m_hw->pbuf);
        int planes = CVPixelBufferGetPlaneCount(m_hw->pbuf);
        for (int i = 0; i < planes; ++i) {
            int w = IOSurfaceGetWidthOfPlane(surface, i);
            int h = IOSurfaceGetHeightOfPlane(surface, i) ;
            MTLPixelFormat f = i ?  MTLPixelFormatRG8Unorm : MTLPixelFormatR8Unorm;
            MTLTextureDescriptor *desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:f width:w height:h mipmapped:NO];

            textures[i] = quint64([m_hw->device newTextureWithDescriptor:desc iosurface:surface plane:i]);
        }

        return textures;
    }

    QAVHWDevice_VideoToolboxPrivate *m_hw = nullptr;
};

QAVVideoBuffer *QAVHWDevice_VideoToolbox::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_MTL(d_ptr.get(), frame);
}

QT_END_NAMESPACE
