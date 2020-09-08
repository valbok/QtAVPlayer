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
