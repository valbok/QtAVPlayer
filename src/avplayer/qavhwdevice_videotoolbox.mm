/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qavhwdevice_videotoolbox_p.h"
#include "qavvideocodec_p.h"
#include "qavplanarvideobuffer_gpu_p.h"
#include <QVideoFrame>
#include <QDebug>

#import <CoreVideo/CoreVideo.h>
#if defined(Q_OS_MACOS)
#import <IOSurface/IOSurface.h>
#else
#import <IOSurface/IOSurfaceRef.h>
#endif
#import <Metal/Metal.h>

QT_BEGIN_NAMESPACE

class QAVHWDevice_VideoToolboxPrivate
{
public:
    id<MTLDevice> device = nullptr;
    CVPixelBufferRef pbuf = nullptr;
};

QAVHWDevice_VideoToolbox::QAVHWDevice_VideoToolbox(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVHWDevice_VideoToolboxPrivate)
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

bool QAVHWDevice_VideoToolbox::supportsVideoSurface(QAbstractVideoSurface *surface) const
{
    if (!surface)
        return false;

    auto list = surface->supportedPixelFormats(QAbstractVideoBuffer::MTLTextureHandle);
    return list.contains(QVideoFrame::Format_NV12);
}

class VideoBuffer_MTL : public QAVPlanarVideoBuffer_GPU
{
public:
    VideoBuffer_MTL(QAVHWDevice_VideoToolboxPrivate *hw, const QAVVideoFrame &frame)
        : QAVPlanarVideoBuffer_GPU(frame, MTLTextureHandle)
        , m_hw(hw)
    {
    }

    QVariant handle() const override
    {
        CVPixelBufferRelease(m_hw->pbuf);
        m_hw->pbuf = (CVPixelBufferRef)frame().frame()->data[3];
        CVPixelBufferRetain(m_hw->pbuf);
        QList<QVariant> textures(2);

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

QVideoFrame QAVHWDevice_VideoToolbox::decode(const QAVVideoFrame &frame) const
{
    return {new VideoBuffer_MTL(d_ptr.data(), frame), frame.size(), QVideoFrame::Format_NV12};
}

QT_END_NAMESPACE
