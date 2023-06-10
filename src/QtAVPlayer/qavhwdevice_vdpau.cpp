/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_vdpau_p.h"
#include "qavvideocodec_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <QDebug>

#include <GL/glx.h>

extern "C" {
#include <libavutil/hwcontext_vdpau.h>
#include <libavcodec/avcodec.h>
}

typedef void (*VDPAUInitNV_)(const GLvoid *, const GLvoid *);
static VDPAUInitNV_ s_VDPAUInitNV = nullptr;

typedef void (*VDPAUFiniNV_)(void);
static VDPAUFiniNV_ s_VDPAUFiniNV = nullptr;

typedef GLvdpauSurfaceNV (*VDPAURegisterOutputSurfaceNV_)(GLvoid *, GLenum, GLsizei, const GLuint *);
static VDPAURegisterOutputSurfaceNV_ s_VDPAURegisterOutputSurfaceNV = nullptr;

typedef void (*VDPAUSurfaceAccessNV_)(GLvdpauSurfaceNV, GLenum);
static VDPAUSurfaceAccessNV_ s_VDPAUSurfaceAccessNV = nullptr;

typedef void (*VDPAUMapSurfacesNV_)(GLsizei, const GLvdpauSurfaceNV *);
static VDPAUMapSurfacesNV_ s_VDPAUMapSurfacesNV = nullptr;

typedef void (*VDPAUUnmapSurfacesNV_)(GLsizei, const GLvdpauSurfaceNV *);
static VDPAUUnmapSurfacesNV_ s_VDPAUUnmapSurfacesNV = nullptr;

typedef void (*VDPAUUnregisterSurfaceNV_)(GLvdpauSurfaceNV);
static VDPAUUnregisterSurfaceNV_ s_VDPAUUnregisterSurfaceNV = nullptr;

static VdpOutputSurfaceCreate *s_output_surface_create = nullptr;
static VdpOutputSurfaceDestroy *s_output_surface_destroy = nullptr;
static VdpVideoMixerCreate *s_video_mixer_create = nullptr;
static VdpVideoMixerDestroy *s_video_mixer_destroy = nullptr;
static VdpVideoSurfaceGetParameters *s_video_surface_get_parameters = nullptr;
static VdpVideoMixerRender *s_video_mixer_render = nullptr;

#define VDP_NUM_MIXER_PARAMETER 3
#define MAX_NUM_FEATURES 6
#define MP_VDP_HISTORY_FRAMES 2

QT_BEGIN_NAMESPACE

QAVHWDevice_VDPAU::QAVHWDevice_VDPAU()
{
}

QAVHWDevice_VDPAU::~QAVHWDevice_VDPAU()
{
}

AVPixelFormat QAVHWDevice_VDPAU::format() const
{
    return AV_PIX_FMT_VDPAU;
}

AVHWDeviceType QAVHWDevice_VDPAU::type() const
{
    return AV_HWDEVICE_TYPE_VDPAU;
}

template <class T>
static int get_proc_address(AVVDPAUDeviceContext *vactx, VdpFuncId funcid, T &out)
{
    void *tmp = nullptr;
    auto err = vactx->get_proc_address(vactx->device, funcid, &tmp);
    if (err != VDP_STATUS_OK)
        qWarning() << "Could not get proc address:" << funcid;
    out = reinterpret_cast<T>(tmp);
    return err;
}

class VideoBuffer_VDPAU_GLX : public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_VDPAU_GLX(const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
    {
        if (!s_VDPAUInitNV) {
            s_VDPAUInitNV = (VDPAUInitNV_) glXGetProcAddressARB((const GLubyte *)"VDPAUInitNV");
            s_VDPAUFiniNV = (VDPAUFiniNV_) glXGetProcAddressARB((const GLubyte *)"VDPAUFiniNV");
            s_VDPAURegisterOutputSurfaceNV = (VDPAURegisterOutputSurfaceNV_) glXGetProcAddressARB((const GLubyte *)"VDPAURegisterOutputSurfaceNV");
            s_VDPAUSurfaceAccessNV = (VDPAUSurfaceAccessNV_) glXGetProcAddressARB((const GLubyte *)"VDPAUSurfaceAccessNV");
            s_VDPAUMapSurfacesNV = (VDPAUMapSurfacesNV_) glXGetProcAddressARB((const GLubyte *)"VDPAUMapSurfacesNV");
            s_VDPAUUnmapSurfacesNV = (VDPAUUnmapSurfacesNV_) glXGetProcAddressARB((const GLubyte *)"VDPAUUnmapSurfacesNV");
            s_VDPAUUnregisterSurfaceNV = (VDPAUUnregisterSurfaceNV_) glXGetProcAddressARB((const GLubyte *)"VDPAUUnregisterSurfaceNV");
        }

        for (int n = 0; n < MP_VDP_HISTORY_FRAMES; ++n)
            past[n] = future[n] = VDP_INVALID_HANDLE;
    }

    ~VideoBuffer_VDPAU_GLX()
    {
        if (vdpgl_surface) {
            s_VDPAUUnmapSurfacesNV(1, &vdpgl_surface);
            s_VDPAUUnregisterSurfaceNV(vdpgl_surface);
        }

        if (vdp_surface != VDP_INVALID_HANDLE)
            s_output_surface_destroy(vdp_surface);

        if (video_mixer != VDP_INVALID_HANDLE)
            s_video_mixer_destroy(video_mixer);

        if (gl_texture) {
            glDeleteTextures(1, &gl_texture);
            s_VDPAUFiniNV();
        }
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::GLTextureHandle;
    }

    GLuint texture()
    {
        if (!s_VDPAUInitNV) {
            qWarning() << "Could not get proc address: s_VDPAUInitNV";
            return 0;
        }

        if (!frame())
            return 0;

        if (gl_texture)
            return gl_texture;

        auto av_frame = frame().frame();
        int w = av_frame->width;
        int h = av_frame->height;

        AVHWDeviceContext *hwctx = (AVHWDeviceContext *)frame().stream().codec()->avctx()->hw_device_ctx->data;
        AVVDPAUDeviceContext *vactx = (AVVDPAUDeviceContext *)hwctx->hwctx;
        VdpVideoSurface surf = (VdpVideoSurface)(uintptr_t)av_frame->data[3];

        if (!gl_texture) {
            if (!s_output_surface_create) {
                get_proc_address(vactx, VDP_FUNC_ID_OUTPUT_SURFACE_CREATE, s_output_surface_create);
                get_proc_address(vactx, VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY, s_output_surface_destroy);
                get_proc_address(vactx, VDP_FUNC_ID_VIDEO_MIXER_CREATE, s_video_mixer_create);
                get_proc_address(vactx, VDP_FUNC_ID_VIDEO_MIXER_DESTROY, s_video_mixer_destroy);
                get_proc_address(vactx, VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS, s_video_surface_get_parameters);
                get_proc_address(vactx, VDP_FUNC_ID_VIDEO_MIXER_RENDER, s_video_mixer_render);
                get_proc_address(vactx, VDP_FUNC_ID_VIDEO_MIXER_RENDER, s_video_mixer_render);
            }

            VdpVideoMixerFeature features[MAX_NUM_FEATURES];
            static const VdpVideoMixerParameter parameters[VDP_NUM_MIXER_PARAMETER] = {
                VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH,
                VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
                VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE,
            };

            VdpChromaType chroma_type;
            uint32_t s_w, s_h;

            s_video_surface_get_parameters(surf, &chroma_type, &s_w, &s_h);
            const void *const parameter_values[VDP_NUM_MIXER_PARAMETER] = {
                &s_w,
                &s_h,
                &chroma_type,
            };

            s_video_mixer_create(vactx->device, 0, features,
                                 VDP_NUM_MIXER_PARAMETER,
                                 parameters, parameter_values,
                                 &video_mixer);

            glGenTextures(1, &gl_texture);
            s_VDPAUInitNV(reinterpret_cast<const GLvoid *>(vactx->device), (const GLvoid *)vactx->get_proc_address);
            glBindTexture(GL_TEXTURE_2D, gl_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);

            s_output_surface_create(vactx->device,
                                    VDP_RGBA_FORMAT_B8G8R8A8,
                                    s_w,
                                    s_h,
                                    &vdp_surface);

            vdpgl_surface = s_VDPAURegisterOutputSurfaceNV(reinterpret_cast<GLvoid *>(vdp_surface),
                                                           GL_TEXTURE_2D,
                                                           1, &gl_texture);

            s_VDPAUSurfaceAccessNV(vdpgl_surface, GL_READ_ONLY);
        }

        VdpRect video_rect = {0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
        s_video_mixer_render(video_mixer, VDP_INVALID_HANDLE,
                             0, field,
                             MP_VDP_HISTORY_FRAMES, past,
                             surf,
                             MP_VDP_HISTORY_FRAMES, future,
                             &video_rect,
                             vdp_surface, NULL, NULL,
                             0, NULL);
        s_VDPAUMapSurfacesNV(1, &vdpgl_surface);
        return gl_texture;
    }

    QVariant handle(QRhi */*rhi*/) const override
    {
        return const_cast<VideoBuffer_VDPAU_GLX *>(this)->texture();
    }

    GLuint gl_texture = 0;
    VdpOutputSurface vdp_surface = VDP_INVALID_HANDLE;
    GLvdpauSurfaceNV vdpgl_surface = 0;
    VdpVideoMixer video_mixer = VDP_INVALID_HANDLE;
    VdpVideoSurface past[MP_VDP_HISTORY_FRAMES];
    VdpVideoSurface future[MP_VDP_HISTORY_FRAMES];
    VdpVideoMixerPictureStructure field = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
};

QAVVideoBuffer *QAVHWDevice_VDPAU::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_VDPAU_GLX(frame);
}

QT_END_NAMESPACE
