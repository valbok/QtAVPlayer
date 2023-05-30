/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavframecodec_p.h"
#include "qavcodec_p_p.h"

#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

QT_BEGIN_NAMESPACE

QAVFrameCodec::QAVFrameCodec()
{
}

QAVFrameCodec::QAVFrameCodec(QAVCodecPrivate &d)
    : QAVCodec(d)
{
}

int QAVFrameCodec::write(const QAVPacket &pkt)
{
    Q_D(QAVCodec);
    if (!d->avctx)
        return AVERROR(EINVAL);
    return avcodec_send_packet(d->avctx, pkt ? pkt.packet() : nullptr);
}

int QAVFrameCodec::read(QAVStreamFrame &frame)
{
    Q_D(QAVCodec);
    if (!d->avctx)
        return AVERROR(EINVAL);
    auto f = static_cast<QAVFrame *>(&frame);
    return avcodec_receive_frame(d->avctx, f->frame());
}

QT_END_NAMESPACE
