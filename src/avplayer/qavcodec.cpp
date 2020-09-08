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

#include "qavcodec_p.h"
#include "qavcodec_p_p.h"

#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

QT_BEGIN_NAMESPACE

QAVCodec::QAVCodec(QObject *parent)
    : QAVCodec(*new QAVCodecPrivate, parent)
{
}

QAVCodec::QAVCodec(QAVCodecPrivate &d, QObject *parent)
    : QObject(parent)
    , d_ptr(&d)
{
}

QAVCodec::~QAVCodec()
{
    Q_D(QAVCodec);
    if (d->avctx)
        avcodec_free_context(&d->avctx);
}

bool QAVCodec::open(AVStream *stream)
{
    Q_D(QAVCodec);

    d->avctx = avcodec_alloc_context3(nullptr);
    if (!stream || !d->avctx)
        return false;

    int ret = avcodec_parameters_to_context(d->avctx, stream->codecpar);
    if (ret < 0) {
        qWarning() << "Failed avcodec_parameters_to_context:" << ret;
        return false;
    }

    d->avctx->pkt_timebase = stream->time_base;
    d->avctx->framerate = stream->avg_frame_rate;
    if (!d->codec)
        d->codec = avcodec_find_decoder(d->avctx->codec_id);
    if (!d->codec) {
        qWarning() << "No decoder could be found for codec";
        return false;
    }

    d->avctx->codec_id = d->codec->id;
    d->avctx->lowres = d->codec->max_lowres;

    av_opt_set_int(d->avctx, "refcounted_frames", true, 0);
    av_opt_set_int(d->avctx, "threads", 1, 0);
    ret = avcodec_open2(d->avctx, d->codec, nullptr);
    if (ret < 0) {
        qWarning() << "Could not open the codec:" << ret;
        return false;
    }

    stream->discard = AVDISCARD_DEFAULT;
    d->stream = stream;

    return true;
}

AVCodecContext *QAVCodec::avctx() const
{
    return d_func()->avctx;
}

AVCodec *QAVCodec::codec() const
{
    return d_func()->codec;
}

AVStream *QAVCodec::stream() const
{
    return d_func()->stream;
}

QAVFrame QAVCodec::decode(const AVPacket *pkt) const
{
    Q_D(const QAVCodec);
    if (!d->avctx || avcodec_send_packet(d->avctx, pkt) < 0)
        return {};

    QAVFrame frame(this);
    avcodec_receive_frame(d->avctx, frame.frame());
    return frame;
}

QT_END_NAMESPACE
