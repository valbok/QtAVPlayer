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

    int sent = 0;
    do {
        sent = avcodec_send_packet(d->avctx, pkt.packet());
        // AVERROR(EAGAIN): input is not accepted in the current state - user must read output with avcodec_receive_frame()
        // (once all output is read, the packet should be resent, and the call will not fail with EAGAIN)
        if (sent < 0 && sent != AVERROR(EAGAIN))
            return false;

        while (true) {
            QAVFrame frame;
            int ret = avcodec_receive_frame(d->avctx, frame.frame());
            // AVERROR(EAGAIN): output is not available in this state - user must try to send new input
            if (ret < 0)
                break;
            frames.push_back(frame);
        }
    } while (sent == AVERROR(EAGAIN));

    return true;
}

QT_END_NAMESPACE
