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

#include "qavhwdevice_vaapi_drm_egl_p.h"
#include "qavvideocodec_p.h"
#include "qavplanarvideobuffer_gpu_p.h"
#include <QVideoFrame>
#include <QDebug>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <va/va_drmcommon.h>
#include <drm/drm_fourcc.h>

extern "C" {
#include <libavutil/hwcontext_vaapi.h>
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

QAVHWDevice_VAAPI_DRM_EGL::QAVHWDevice_VAAPI_DRM_EGL(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVHWDevice_VAAPI_DRM_EGLPrivate)
{
}

QAVHWDevice_VAAPI_DRM_EGL::~QAVHWDevice_VAAPI_DRM_EGL()
{
    Q_D(QAVHWDevice_VAAPI_DRM_EGL);

    glDeleteTextures(2, d->textures);
}

AVPixelFormat QAVHWDevice_VAAPI_DRM_EGL::format() const
{
    return AV_PIX_FMT_VAAPI;
}

bool QAVHWDevice_VAAPI_DRM_EGL::supportsVideoSurface(QAbstractVideoSurface *surface) const
{
    if (!surface)
        return false;

    auto list = surface->supportedPixelFormats(QAbstractVideoBuffer::GLTextureHandle);
    return list.contains(QVideoFrame::Format_NV12);
}

class VideoBuffer_EGL : public QAVPlanarVideoBuffer_GPU
{
public:
    VideoBuffer_EGL(QAVHWDevice_VAAPI_DRM_EGLPrivate *hw, const QAVVideoFrame &frame)
        : QAVPlanarVideoBuffer_GPU(frame, GLTextureHandle)
        , m_hw(hw)
    {
        if (!s_eglCreateImageKHR) {
            s_eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
            s_eglDestroyImageKHR = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
            s_glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
        }
    }

    QVariant textures() const
    {
        return QList<QVariant>() << m_hw->textures[0] << m_hw->textures[1];
    }

    QVariant handle() const override
    {
        if (!m_hw->textures[0])
            glGenTextures(2, m_hw->textures);

        auto va_frame = frame().frame();
        AVHWDeviceContext *hwctx = (AVHWDeviceContext *)frame().codec()->avctx()->hw_device_ctx->data;
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

QVideoFrame QAVHWDevice_VAAPI_DRM_EGL::decode(const QAVVideoFrame &frame) const
{
    return {new VideoBuffer_EGL(d_ptr.data(), frame), frame.size(), QVideoFrame::Format_NV12};
}

QT_END_NAMESPACE
