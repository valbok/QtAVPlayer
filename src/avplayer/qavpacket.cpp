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

#include "qavpacket_p.h"
#include "qavcodec_p.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

QT_BEGIN_NAMESPACE

class QAVPacketPrivate
{
public:
    const QAVCodec *codec = nullptr;
    AVPacket pkt;
    QAVFrame frame;
};

QAVPacket::QAVPacket(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVPacketPrivate)
{
    av_init_packet(&d_ptr->pkt);
    d_ptr->pkt.size = 0;
    d_ptr->pkt.stream_index = -1;
}

QAVPacket::QAVPacket(const QAVPacket &other)
    : QAVPacket()
{
    *this = other;
}

QAVPacket &QAVPacket::operator=(const QAVPacket &other)
{
    av_packet_unref(&d_ptr->pkt);
    av_packet_ref(&d_ptr->pkt, &other.d_ptr->pkt);
    d_ptr->codec = other.d_ptr->codec;
    d_ptr->frame = other.d_ptr->frame;
    return *this;
}

QAVPacket::operator bool() const
{
    Q_D(const QAVPacket);
    return d->pkt.size;
}

QAVPacket::~QAVPacket()
{
    Q_D(QAVPacket);
    av_packet_unref(&d->pkt);
}

AVPacket *QAVPacket::packet()
{
    return &d_func()->pkt;
}

double QAVPacket::duration() const
{
    Q_D(const QAVPacket);
    if (!d->codec || !d->codec->stream())
        return 0;

    return d->pkt.duration * av_q2d(d->codec->stream()->time_base);
}

int QAVPacket::bytes() const
{
    return d_func()->pkt.size;
}

int QAVPacket::streamIndex() const
{
    return d_func()->pkt.stream_index;
}

void QAVPacket::setCodec(const QAVCodec *c)
{
    Q_D(QAVPacket);
    d->codec = c;
}

QAVFrame QAVPacket::decode()
{
    Q_D(QAVPacket);
    if (!d->codec)
        return {};

    if (d->frame)
        return d->frame;

    d->frame = d->codec->decode(&d->pkt);
    return d->frame;
}

QT_END_NAMESPACE
