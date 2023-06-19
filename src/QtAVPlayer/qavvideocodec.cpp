/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideocodec_p.h"
#include "qavhwdevice_p.h"
#include "qavcodec_p_p.h"
#include "qavpacket_p.h"
#include "qavframe.h"
#include "qavvideoframe.h"
#include <QDebug>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
}

QT_BEGIN_NAMESPACE

class QAVVideoCodecPrivate : public QAVCodecPrivate
{
public:
    QSharedPointer<QAVHWDevice> hw_device;
};

static bool isSoftwarePixelFormat(AVPixelFormat from)
{
    switch (from) {
    case AV_PIX_FMT_VAAPI:
    case AV_PIX_FMT_VDPAU:
    case AV_PIX_FMT_MEDIACODEC:
    case AV_PIX_FMT_VIDEOTOOLBOX:
    case AV_PIX_FMT_D3D11:
    case AV_PIX_FMT_D3D11VA_VLD:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 0, 0)
    case AV_PIX_FMT_OPENCL:
#endif
    case AV_PIX_FMT_CUDA:
    case AV_PIX_FMT_DXVA2_VLD:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(52, 58, 101)
    case AV_PIX_FMT_XVMC:
#endif
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 134, 0)
    case AV_PIX_FMT_VULKAN:
#endif
    case AV_PIX_FMT_DRM_PRIME:
    case AV_PIX_FMT_MMAL:
    case AV_PIX_FMT_QSV:
        return false;
    default:
        return true;
    }
}

static AVPixelFormat negotiate_pixel_format(AVCodecContext *c, const AVPixelFormat *f)
{
    auto d = reinterpret_cast<QAVVideoCodecPrivate *>(c->opaque);

    QList<AVHWDeviceType> supported;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 0, 0)
    for (int i = 0;; ++i) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(c->codec, i);
        if (!config)
            break;

        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
            supported.append(config->device_type);
    }

    if (!supported.isEmpty()) {
        qDebug() << c->codec->name << ": supported hardware device contexts:";
        for (auto a: supported)
            qDebug() << "   " << av_hwdevice_get_type_name(a);
    } else {
        qWarning() << "None of the hardware accelerations are supported";
    }
#endif

    QList<AVPixelFormat> softwareFormats;
    QList<AVPixelFormat> hardwareFormats;
    for (int i = 0; f[i] != AV_PIX_FMT_NONE; ++i) {
        if (!isSoftwarePixelFormat(f[i])) {
            hardwareFormats.append(f[i]);
            continue;
        }
        softwareFormats.append(f[i]);
    }

    qDebug() << "Available pixel formats:";
    for (auto a : softwareFormats) {
        auto dsc = av_pix_fmt_desc_get(a);
        qDebug() << "  " << dsc->name << ": AVPixelFormat(" << a << ")";
    }

    for (auto a : hardwareFormats) {
        auto dsc = av_pix_fmt_desc_get(a);
        qDebug() << "  " << dsc->name << ": AVPixelFormat(" << a << ")";
    }

    AVPixelFormat pf = !softwareFormats.isEmpty() ? softwareFormats[0] : AV_PIX_FMT_NONE;
    const char *decStr = "software";
    if (d->hw_device) {
        for (auto f : hardwareFormats) {
            if (f == d->hw_device->format()) {
                d->hw_device->init(c);
                pf = d->hw_device->format();
                decStr = "hardware";
                break;
            }
        }
    }

    auto dsc = av_pix_fmt_desc_get(pf);
    if (dsc)
        qDebug() << "Using" << decStr << "decoding in" << dsc->name;
    else
        qDebug() << "None of the pixel formats";

    return pf;
}

QAVVideoCodec::QAVVideoCodec()
    : QAVFrameCodec(*new QAVVideoCodecPrivate)
{
    d_ptr->avctx->opaque = d_ptr.get();
    d_ptr->avctx->get_format = negotiate_pixel_format;
}

QAVVideoCodec::~QAVVideoCodec()
{
    av_buffer_unref(&avctx()->hw_device_ctx);
}

void QAVVideoCodec::setDevice(const QSharedPointer<QAVHWDevice> &d)
{
    d_func()->hw_device = d;
}

QAVHWDevice *QAVVideoCodec::device() const
{
    return d_func()->hw_device.data();
}

QT_END_NAMESPACE
