/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_vaapi_drm_egl_p.h"
#include "qavvideocodec_p.h"
#include "qavvideobuffer_gpu_p.h"
#include "qavstream.h"
#include <QDebug>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <va/va_drmcommon.h>
#include <drm/drm_fourcc.h>

extern "C" {
#include <libavutil/hwcontext_vaapi.h>
#include <libavcodec/avcodec.h>
}

static PFNEGLCREATEIMAGEKHRPROC s_eglCreateImageKHR = nullptr;
static PFNEGLDESTROYIMAGEKHRPROC s_eglDestroyImageKHR = nullptr;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC s_glEGLImageTargetTexture2DOES = nullptr;

QT_BEGIN_NAMESPACE

class QAVHWDevice_VAAPI_DRM_EGLPrivate
{
public:
    GLuint textures[2] = {0};
};

QAVHWDevice_VAAPI_DRM_EGL::QAVHWDevice_VAAPI_DRM_EGL()
    : d_ptr(new QAVHWDevice_VAAPI_DRM_EGLPrivate)
{
}

QAVHWDevice_VAAPI_DRM_EGL::~QAVHWDevice_VAAPI_DRM_EGL()
{
    Q_D(QAVHWDevice_VAAPI_DRM_EGL);

    if (d->textures[0])
        glDeleteTextures(2, &d->textures[0]);
}

AVPixelFormat QAVHWDevice_VAAPI_DRM_EGL::format() const
{
    return AV_PIX_FMT_VAAPI;
}

AVHWDeviceType QAVHWDevice_VAAPI_DRM_EGL::type() const
{
    return AV_HWDEVICE_TYPE_VAAPI;
}

class VideoBuffer_EGL : public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_EGL(QAVHWDevice_VAAPI_DRM_EGLPrivate *hw, const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
        , m_hw(hw)
    {
        if (!s_eglCreateImageKHR) {
            s_eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
            s_eglDestroyImageKHR = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
            s_glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
        }
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::GLTextureHandle;
    }

    QVariant textures() const
    {
        return QList<QVariant>() << m_hw->textures[0] << m_hw->textures[1];
    }

    QVariant handle(QRhi */*rhi*/) const override
    {
        if (!m_hw->textures[0])
            glGenTextures(2, m_hw->textures);

        auto va_frame = frame().frame();
        AVHWDeviceContext *hwctx = (AVHWDeviceContext *)frame().stream().codec()->avctx()->hw_device_ctx->data;
        AVVAAPIDeviceContext *vactx = (AVVAAPIDeviceContext *)hwctx->hwctx;
        VADisplay va_display = vactx->display;
        VASurfaceID va_surface = (VASurfaceID)(uintptr_t)va_frame->data[3];

        VADRMPRIMESurfaceDescriptor prime;
        auto status = vaExportSurfaceHandle(va_display, va_surface,
                                            VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                                            VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                                            &prime);
        if (status != VA_STATUS_SUCCESS) {
            qWarning() << "vaExportSurfaceHandle failed" << status;
            return textures();
        }

        if (prime.fourcc != VA_FOURCC_NV12) {
            qWarning() << "prime.fourcc != VA_FOURCC_NV12";
            return textures();
        }

        vaSyncSurface(va_display, va_surface);

        static const uint32_t formats[2] = { DRM_FORMAT_R8, DRM_FORMAT_GR88 };
        for (int i = 0; i < 2; ++i) {
            if (prime.layers[i].drm_format != formats[i])
                qWarning() << "Wrong DRM format:" << prime.layers[i].drm_format << formats[i];

            EGLint img_attr[] = {
                EGL_LINUX_DRM_FOURCC_EXT,      EGLint(formats[i]),
                EGL_WIDTH,                     va_frame->width / (i + 1),
                EGL_HEIGHT,                    va_frame->height / (i + 1),
                EGL_DMA_BUF_PLANE0_FD_EXT,     prime.objects[prime.layers[i].object_index[0]].fd,
                EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGLint(prime.layers[i].offset[0]),
                EGL_DMA_BUF_PLANE0_PITCH_EXT,  EGLint(prime.layers[i].pitch[0]),
                EGL_NONE
            };

            EGLImage img = s_eglCreateImageKHR(eglGetCurrentDisplay(),
                                               EGL_NO_CONTEXT,
                                               EGL_LINUX_DMA_BUF_EXT,
                                               NULL, img_attr);
            if (!img) {
                qWarning() << "eglCreateImageKHR failed";
                return textures();
            }

            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, m_hw->textures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            s_glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, img);
            if (glGetError())
                qWarning() << "glEGLImageTargetTexture2DOES failed";

            glBindTexture(GL_TEXTURE_2D, 0);
            s_eglDestroyImageKHR(eglGetCurrentDisplay(), img);
        }

        return textures();
    }

    QAVHWDevice_VAAPI_DRM_EGLPrivate *m_hw = nullptr;
};

QAVVideoBuffer *QAVHWDevice_VAAPI_DRM_EGL::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_EGL(d_ptr.get(), frame);
}

QT_END_NAMESPACE
