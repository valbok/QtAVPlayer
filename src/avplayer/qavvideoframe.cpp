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

#include "qavvideoframe_p.h"
#include "qavplanarvideobuffer_cpu_p.h"
#include "qavframe_p_p.h"
#include "qavvideocodec_p.h"
#include "qavhwdevice_p.h"
#include <QVideoFrame>
#include <QDebug>

QT_BEGIN_NAMESPACE

QAVVideoFrame::QAVVideoFrame(QObject *parent)
    : QAVFrame(parent)
{
}

const QAVVideoCodec *QAVVideoFrame::codec() const
{
    return reinterpret_cast<const QAVVideoCodec *>(d_func()->codec);
}

QAVVideoFrame::QAVVideoFrame(const QAVFrame &other, QObject *parent)
    : QAVVideoFrame(parent)
{
    operator=(other);
}

QAVVideoFrame &QAVVideoFrame::operator=(const QAVFrame &other)
{
    QAVFrame::operator=(other);
    return *this;
}

QVideoFrame::PixelFormat QAVVideoFrame::pixelFormat(AVPixelFormat from)
{
    switch (from) {
    case AV_PIX_FMT_YUV420P:
        return QVideoFrame::Format_YUV420P;
    case AV_PIX_FMT_NV12:
        return QVideoFrame::Format_NV12;
    case AV_PIX_FMT_BGRA:
        return QVideoFrame::Format_BGRA32;
    case AV_PIX_FMT_ARGB:
        return QVideoFrame::Format_ARGB32;
    default:
        return QVideoFrame::Format_Invalid;
    }
}

AVPixelFormat QAVVideoFrame::pixelFormat(QVideoFrame::PixelFormat from)
{
    switch (from) {
    case QVideoFrame::Format_YUV420P:
        return AV_PIX_FMT_YUV420P;
    case QVideoFrame::Format_NV12:
        return AV_PIX_FMT_NV12;
    default:
        return AV_PIX_FMT_NONE;
    }
}

QSize QAVVideoFrame::size() const
{
    Q_D(const QAVFrame);
    return {d->frame->width, d->frame->height};
}

QAVVideoFrame::operator QVideoFrame() const
{
    Q_D(const QAVFrame);
    if (!codec())
        return {};

    if (codec()->device())
        return codec()->device()->decode(*this);

    auto format = pixelFormat(AVPixelFormat(d->frame->format));
    return {new QAVPlanarVideoBuffer_CPU(*this), size(), format};
}

QT_END_NAMESPACE
