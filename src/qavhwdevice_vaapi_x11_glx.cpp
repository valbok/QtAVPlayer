/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_vaapi_x11_glx_p.h"
#include "qavvideocodec_p.h"
#include "qavstream.h"
#include "qavvideobuffer_gpu_p.h"
#include <QDebug>

#include <GL/glx.h>
#include <va/va_x11.h>

extern "C" {
#include <libavutil/hwcontext_vaapi.h>
#include <libavcodec/avcodec.h>
}

typedef void (*glXBindTexImageEXT_)(Display *dpy, GLXDrawable drawable, int buffer, const int *attrib_list);
typedef void (*glXReleaseTexImageEXT_)(Display *dpy, GLXDrawable draw, int buffer);
static glXBindTexImageEXT_ s_glXBindTexImageEXT = nullptr;
static glXReleaseTexImageEXT_ s_glXReleaseTexImageEXT = nullptr;

QT_BEGIN_NAMESPACE

class QAVHWDevice_VAAPI_X11_GLXPrivate
{
public:
    Pixmap pixmap = 0;
    GLXPixmap glxpixmap = 0;
    Display *display = nullptr;
    GLuint texture = 0;
};

QAVHWDevice_VAAPI_X11_GLX::QAVHWDevice_VAAPI_X11_GLX()
    : d_ptr(new QAVHWDevice_VAAPI_X11_GLXPrivate)
{
}

QAVHWDevice_VAAPI_X11_GLX::~QAVHWDevice_VAAPI_X11_GLX()
{
    Q_D(QAVHWDevice_VAAPI_X11_GLX);

    if (d->glxpixmap) {
        s_glXReleaseTexImageEXT(d->display, d->glxpixmap, GLX_FRONT_EXT);
        glXDestroyPixmap(d->display, d->glxpixmap);
    }
    if (d->pixmap)
        XFreePixmap(d->display, d->pixmap);

    if (d->texture)
        glDeleteTextures(1, &d->texture);
}

AVPixelFormat QAVHWDevice_VAAPI_X11_GLX::format() const
{
    return AV_PIX_FMT_VAAPI;
}

AVHWDeviceType QAVHWDevice_VAAPI_X11_GLX::type() const
{
    return AV_HWDEVICE_TYPE_VAAPI;
}

class VideoBuffer_GLX : public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_GLX(QAVHWDevice_VAAPI_X11_GLXPrivate *hw, const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
        , m_hw(hw)
    {
        if (!s_glXBindTexImageEXT) {
            s_glXBindTexImageEXT = (glXBindTexImageEXT_) glXGetProcAddressARB((const GLubyte *)"glXBindTexImageEXT");
            s_glXReleaseTexImageEXT = (glXReleaseTexImageEXT_) glXGetProcAddressARB((const GLubyte *)"glXReleaseTexImageEXT");
        }
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::GLTextureHandle;
    }

    QVariant handle(QRhi */*rhi*/) const override
    {
        if (!s_glXBindTexImageEXT) {
            qWarning() << "Could not get proc address: s_glXBindTexImageEXT";
            return 0;
        }

        auto av_frame = frame().frame();
        AVHWDeviceContext *hwctx = (AVHWDeviceContext *)frame().stream().codec()->avctx()->hw_device_ctx->data;
        AVVAAPIDeviceContext *vactx = (AVVAAPIDeviceContext *)hwctx->hwctx;
        VADisplay va_display = vactx->display;
        VASurfaceID va_surface = (VASurfaceID)(uintptr_t)av_frame->data[3];

        int w = av_frame->width;
        int h = av_frame->height;

        if (!m_hw->display) {
            glGenTextures(1, &m_hw->texture);
            auto display = (Display *)glXGetCurrentDisplay();
            m_hw->display = display;
            int xscr = DefaultScreen(display);
            const char *glxext = glXQueryExtensionsString(display, xscr);
            if (!glxext || !strstr(glxext, "GLX_EXT_texture_from_pixmap")) {
                qWarning() << "GLX_EXT_texture_from_pixmap is not supported";
                return 0;
            }

            int attribs[] = {
                GLX_RENDER_TYPE, GLX_RGBA_BIT,
                GLX_X_RENDERABLE, True,
                GLX_BIND_TO_TEXTURE_RGBA_EXT, True,
                GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT,
                GLX_BIND_TO_TEXTURE_TARGETS_EXT, GLX_TEXTURE_2D_BIT_EXT,
                GLX_Y_INVERTED_EXT, True,
                GLX_DOUBLEBUFFER, False,
                GLX_RED_SIZE, 8,
                GLX_GREEN_SIZE, 8,
                GLX_BLUE_SIZE, 8,
                GLX_ALPHA_SIZE, 8,
                None
            };

            int fbcount;
            GLXFBConfig *fbcs = glXChooseFBConfig(display, xscr, attribs, &fbcount);
            if (!fbcount) {
                XFree(fbcs);
                qWarning() << "No texture-from-pixmap support";
                return 0;
            }

            GLXFBConfig fbc = fbcs[0];
            XFree(fbcs);

            XWindowAttributes xwa;
            XGetWindowAttributes(display, DefaultRootWindow(display), &xwa);

            m_hw->pixmap = XCreatePixmap(display, DefaultRootWindow(display), w, h, xwa.depth);

            const int attribs_pixmap[] = {
                GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
                GLX_TEXTURE_FORMAT_EXT, xwa.depth == 32 ? GLX_TEXTURE_FORMAT_RGBA_EXT : GLX_TEXTURE_FORMAT_RGB_EXT,
                GLX_MIPMAP_TEXTURE_EXT, False,
                None,
            };

            m_hw->glxpixmap = glXCreatePixmap(display, fbc, m_hw->pixmap, attribs_pixmap);
        }

        vaSyncSurface(va_display, va_surface);
        auto status = vaPutSurface(va_display, va_surface, m_hw->pixmap,
                                   0, 0, w, h,
                                   0, 0, w, h,
                                   NULL, 0, VA_FRAME_PICTURE | VA_SRC_BT709);
        if (status != VA_STATUS_SUCCESS) {
            qWarning() << "vaPutSurface failed" << status;
            return 0;
        }

        XSync(m_hw->display, False);
        glBindTexture(GL_TEXTURE_2D, m_hw->texture);
        s_glXBindTexImageEXT(m_hw->display, m_hw->glxpixmap, GLX_FRONT_EXT, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        return m_hw->texture;
    }

    QAVHWDevice_VAAPI_X11_GLXPrivate *m_hw = nullptr;
};

QAVVideoBuffer *QAVHWDevice_VAAPI_X11_GLX::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_GLX(d_ptr.get(), frame);
}

QT_END_NAMESPACE
