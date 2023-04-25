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

QAVFrameCodec::QAVFrameCodec(QObject *parent)
    : QAVCodec(parent)
{
}

QAVFrameCodec::QAVFrameCodec(QAVCodecPrivate &d, QObject *parent)
    : QAVCodec(d, parent)
{
}

bool QAVFrameCodec::decode(const AVPacket *pkt, AVFrame *frame) const
{
    Q_D(const QAVCodec);
    if (!d->avctx)
        return false;

    int ret = avcodec_receive_frame(d->avctx, frame);
    // Frame was returned
    if (ret >= 0)
        return true;
    // Output is not available in this state - user must try to send new input
    if (ret == AVERROR(EAGAIN))
        avcodec_send_packet(d->avctx, pkt);
    // Try next packet
    return false;
}

QT_END_NAMESPACE
