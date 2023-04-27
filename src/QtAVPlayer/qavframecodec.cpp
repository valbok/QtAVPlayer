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

bool QAVFrameCodec::decode(const QAVPacket &pkt, QList<QAVFrame> &frames) const
{
    Q_D(const QAVCodec);
    if (!d->avctx)
        return false;

    int ret = avcodec_send_packet(d->avctx, pkt.packet());
    if (ret < 0 && ret != AVERROR(EAGAIN))
        return false;

    QAVFrame frame;
    while ((ret = avcodec_receive_frame(d->avctx, frame.frame()) >= 0))
        frames.push_back(frame);

    return true;
}

QT_END_NAMESPACE
