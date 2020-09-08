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
#include "qavframe_p.h"
#include "qavvideoframe_p.h"
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
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

QAVVideoCodec::QAVVideoCodec(QObject *parent)
    : QAVCodec(*new QAVVideoCodecPrivate, parent)
{
}

QAVVideoCodec::QAVVideoCodec(AVCodec *c, QObject *parent)
    : QAVVideoCodec(parent)
{
    d_ptr->codec = c;
}

void QAVVideoCodec::setDevice(QAVHWDevice *d)
{
    d_func()->hw_device.reset(d);
}

QAVHWDevice *QAVVideoCodec::device() const
{
    return d_func()->hw_device.data();
}

static AVPixelFormat negotiate_pixel_format(AVCodecContext *c, const AVPixelFormat *f)
{
    auto d = reinterpret_cast<QAVVideoCodecPrivate *>(c->opaque);

    QList<AVPixelFormat> supported;
    QList<AVPixelFormat> unsupported;
    for (int i = 0; f[i] != AV_PIX_FMT_NONE; ++i) {
        auto format = QAVVideoFrame::pixelFormat(f[i]);
        if (format == QVideoFrame::Format_Invalid) {
            unsupported.append(f[i]);
            continue;
        }
        supported.append(f[i]);
    }

    qDebug() << "Available pixel formats:";
    for (auto a : supported) {
        auto dsc = av_pix_fmt_desc_get(a);
        qDebug() << "  " << dsc->name << ": AVPixelFormat(" << a << ") =>" << QAVVideoFrame::pixelFormat(a);
    }

    for (auto a : unsupported) {
        auto dsc = av_pix_fmt_desc_get(a);
        qDebug() << "  " << dsc->name << ": AVPixelFormat(" << a << ")";
    }

    if (d->hw_device) {
        auto dsc = av_pix_fmt_desc_get(d->hw_device->format());
        if (d->hw_device->supportsVideoSurface(d->videoSurface)) {
            qDebug() << "Using hardware decoding in" << dsc->name;
            return d->hw_device->format();
        } else {
            qWarning() << "The video surface does not support hardware device:" << dsc->name;
        }

        d->hw_device.reset();
    }

    for (auto sf : d->videoSurface->supportedPixelFormats(QAbstractVideoBuffer::NoHandle)) {
        auto pf = QAVVideoFrame::pixelFormat(sf);
        if (supported.contains(pf)) {
            qDebug() << "Using software decoding in" << sf;
            return pf;
        }
    }

    qDebug() << "None of the surface's pixel format supported:";
    for (auto a : d->videoSurface->supportedPixelFormats(QAbstractVideoBuffer::NoHandle))
        qDebug() << "  " << a;

    return !supported.isEmpty() ? supported[0] : AV_PIX_FMT_NONE;
}

void QAVVideoCodec::setVideoSurface(QAbstractVideoSurface *surface)
{
    Q_D(QAVVideoCodec);
    if (!surface)
        return;

    d->videoSurface = surface;
    d->avctx->opaque = d;
    d->avctx->get_format = negotiate_pixel_format;
}

QT_END_NAMESPACE
