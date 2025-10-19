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
#include <stdatomic.h>

struct FFAMediaCodec;
struct FFAMediaFormat;
// libavcodec/mediacodecdec_common.h
typedef struct MediaCodecDecContext {

    AVCodecContext *avctx;
    atomic_int refcount;
    atomic_int hw_buffer_count;

    char *codec_name;

    FFAMediaCodec *codec;
    FFAMediaFormat *format;

    void *surface;

    int started;
    int draining;
    int flushing;
    int eos;

    int width;
    int height;
    int stride;
    int slice_height;
    int color_format;
    int crop_top;
    int crop_bottom;
    int crop_left;
    int crop_right;
    int display_width;
    int display_height;

    uint64_t output_buffer_count;
    ssize_t current_input_buffer;

    bool delay_flush;
    atomic_int serial;

    bool use_ndk_codec;
} MediaCodecDecContext;

typedef struct MediaCodecBuffer {

    MediaCodecDecContext *ctx;
    ssize_t index;
    int64_t pts;
    atomic_int released;
    int serial;

} MediaCodecBuffer;
}

QT_BEGIN_NAMESPACE

class QAVHWDevice_MediaCodecPrivate
{
public:
    QAVHWDevice_MediaCodecPrivate()
    {
        androidSurfaceTexture = std::make_unique<QAVAndroidSurfaceTexture>();
    }

    GLuint texture = 0;
    std::unique_ptr<QAVAndroidSurfaceTexture> androidSurfaceTexture;
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
                mediaDeviceContext->surface = d_ptr->androidSurfaceTexture->surface();
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
        if (!m_hw->androidSurfaceTexture->isValid())
            return {};

        if (!m_hw->texture) {
            m_hw->androidSurfaceTexture->detachFromGLContext();
            glGenTextures(1, &m_hw->texture);
            m_hw->androidSurfaceTexture->attachToGLContext(m_hw->texture);
        }

        auto buffer = reinterpret_cast<AVMediaCodecBuffer *>(frame().frame()->data[3]);
        if (!buffer) {
            qWarning() << "Received a frame without AVMediaCodecBuffer.";
        } else if (av_mediacodec_release_buffer(buffer, 1) < 0) {
            qWarning() << "Failed to render buffer to surface.";
            return {};
        }

        m_hw->androidSurfaceTexture->updateTexImage();
        return m_hw->texture;
    }

    QSize size() const override
    {
        auto av_frame = frame().frame();
        auto buffer = reinterpret_cast<AVMediaCodecBuffer *>(av_frame->data[3]);
        if (!buffer) {
            return frame().size();
        }
        auto ctx = reinterpret_cast<MediaCodecDecContext *>(buffer->ctx);
        return {ctx->width, ctx->height};
    }

    QAVHWDevice_MediaCodecPrivate *m_hw = nullptr;
};

QAVVideoBuffer *QAVHWDevice_MediaCodec::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_MediaCodec(d_ptr.get(), frame);
}

QT_END_NAMESPACE
