/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavsubtitlecodec_p.h"
#include "qavcodec_p_p.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

QT_BEGIN_NAMESPACE

QAVSubtitleCodec::QAVSubtitleCodec(QObject *parent)
    : QAVCodec(parent)
{
}

bool QAVSubtitleCodec::decode(const QAVPacket &pkt, QAVSubtitleFrame &frame) const
{
    Q_D(const QAVCodec);

    int got_output = 0;
    int ret = avcodec_decode_subtitle2(
        d->avctx,
        frame.subtitle(),
        &got_output,
        const_cast<AVPacket *>(pkt.packet()));

    if (ret < 0 && ret != AVERROR(EAGAIN))
        return false;

    return got_output;
}

QT_END_NAMESPACE
