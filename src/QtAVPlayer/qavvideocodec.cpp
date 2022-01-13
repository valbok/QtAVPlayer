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
}

QT_BEGIN_NAMESPACE

class QAVVideoCodecPrivate : public QAVCodecPrivate
{
public:
    QScopedPointer<QAVHWDevice> hw_device;
    QAbstractVideoSurface *videoSurface = nullptr;
};

static bool supportedPixelFormat(AVPixelFormat from)
{
    switch (from) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUV420P10LE:
    case AV_PIX_FMT_NV12:
    case AV_PIX_FMT_BGRA:
    case AV_PIX_FMT_ARGB:
        return true;
    default:
        return false;
    }
}

static AVPixelFormat negotiate_pixel_format(AVCodecContext *c, const AVPixelFormat *f)
{
    auto d = reinterpret_cast<QAVVideoCodecPrivate *>(c->opaque);

    QList<AVPixelFormat> supported;
    QList<AVPixelFormat> unsupported;
    for (int i = 0; f[i] != AV_PIX_FMT_NONE; ++i) {
        if (!supportedPixelFormat(f[i])) {
            unsupported.append(f[i]);
            continue;
        }
        supported.append(f[i]);
    }

    qDebug() << "Available pixel formats:";
    for (auto a : supported) {
        auto dsc = av_pix_fmt_desc_get(a);
        qDebug() << "  " << dsc->name << ": AVPixelFormat(" << a << ")";
    }

    for (auto a : unsupported) {
        auto dsc = av_pix_fmt_desc_get(a);
        qDebug() << "  " << dsc->name << ": AVPixelFormat(" << a << ")";
    }

    AVPixelFormat pf = !supported.isEmpty() ? supported[0] : AV_PIX_FMT_NONE;
    const char *decStr = "software";
    if (d->hw_device) {
        for (auto f : unsupported) {
            if (f == d->hw_device->format()) {
                pf = d->hw_device->format();
                decStr = "hardware";
                break;
            }
        }
    }

    auto dsc = av_pix_fmt_desc_get(pf);
    if (dsc)
        qDebug() << "Using" << decStr << "decoding in" << dsc->name;

    return pf;
}

QAVVideoCodec::QAVVideoCodec(QObject *parent)
    : QAVCodec(*new QAVVideoCodecPrivate, parent)
{
    d_ptr->avctx->opaque = d_ptr.data();
    d_ptr->avctx->get_format = negotiate_pixel_format;
}

void QAVVideoCodec::setDevice(QAVHWDevice *d)
{
    d_func()->hw_device.reset(d);
}

QAVHWDevice *QAVVideoCodec::device() const
{
    return d_func()->hw_device.data();
}

QT_END_NAMESPACE
