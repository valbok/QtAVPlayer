/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_mediacodec_p.h"
#include "qavvideobuffer_gpu_p.h"
#include "qavcodec_p.h"
#include "qavandroidsurfacetexture_p.h"
#include <GLES/gl.h>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavcodec/mediacodec.h>
#include <libavutil/hwcontext_mediacodec.h>
}

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QAVAndroidSurfaceTexture, androidSurfaceTexture);

class QAVHWDevice_MediaCodecPrivate
{
public:
    GLuint texture = 0;
};

QAVHWDevice_MediaCodec::QAVHWDevice_MediaCodec()
    : d_ptr(new QAVHWDevice_MediaCodecPrivate)
{
}

QAVHWDevice_MediaCodec::~QAVHWDevice_MediaCodec()
{
    Q_D(QAVHWDevice_MediaCodec);
    if (d->texture)
        glDeleteTextures(1, &d->texture);
}

void QAVHWDevice_MediaCodec::init(AVCodecContext *avctx)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    AVBufferRef *hw_device_ctx = avctx->hw_device_ctx;
    if (hw_device_ctx) {
        AVHWDeviceContext *deviceContext = reinterpret_cast<AVHWDeviceContext *>(hw_device_ctx->data);
        if (deviceContext->hwctx) {
            AVMediaCodecDeviceContext *mediaDeviceContext =
                reinterpret_cast<AVMediaCodecDeviceContext *>(deviceContext->hwctx);
            if (mediaDeviceContext)
                mediaDeviceContext->surface = androidSurfaceTexture->surface();
        }
    }
#else
    Q_UNUSED(avctx);
#endif
}

AVPixelFormat QAVHWDevice_MediaCodec::format() const
{
    return AV_PIX_FMT_MEDIACODEC;
}

AVHWDeviceType QAVHWDevice_MediaCodec::type() const
{
    return AV_HWDEVICE_TYPE_MEDIACODEC;
}

class VideoBuffer_MediaCodec : public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_MediaCodec(QAVHWDevice_MediaCodecPrivate *hw, const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
        , m_hw(hw)
    {
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::GLTextureHandle;
    }

    QVariant handle(QRhi */*rhi*/) const override
    {
        if (!androidSurfaceTexture->isValid())
            return {};

        if (!m_hw->texture) {
            androidSurfaceTexture->detachFromGLContext();
            glGenTextures(1, &m_hw->texture);
            androidSurfaceTexture->attachToGLContext(m_hw->texture);
        }

        AVMediaCodecBuffer *buffer = reinterpret_cast<AVMediaCodecBuffer *>(frame().frame()->data[3]);
        if (!buffer) {
            qWarning() << "Received a frame without AVMediaCodecBuffer.";
        } else if (av_mediacodec_release_buffer(buffer, 1) < 0) {
            qWarning() << "Failed to render buffer to surface.";
            return {};
        }

        androidSurfaceTexture->updateTexImage();
        return m_hw->texture;
    }

    QAVHWDevice_MediaCodecPrivate *m_hw = nullptr;
};

QAVVideoBuffer *QAVHWDevice_MediaCodec::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_MediaCodec(d_ptr.get(), frame);
}

QT_END_NAMESPACE
