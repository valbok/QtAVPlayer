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

class QAVSubtitleCodecPrivate : public QAVCodecPrivate
{
    Q_DECLARE_PUBLIC(QAVSubtitleCodec)
public:
    QAVSubtitleCodecPrivate(QAVSubtitleCodec *q) : q_ptr(q) { }

    QAVSubtitleCodec *q_ptr = nullptr;
    QAVSubtitleFrame frame;
    int gotOutput = 0;
};

QAVSubtitleCodec::QAVSubtitleCodec()
    : QAVCodec(*new QAVSubtitleCodecPrivate(this))
{
}

int QAVSubtitleCodec::write(const QAVPacket &pkt)
{
    Q_D(QAVSubtitleCodec);
    if (!d->avctx)
        return AVERROR(EINVAL);
    d->frame.setStream(pkt.stream());
    return avcodec_decode_subtitle2(
        d->avctx,
        d->frame.subtitle(),
        &d->gotOutput,
        const_cast<AVPacket *>(pkt.packet()));
}

int QAVSubtitleCodec::read(QAVStreamFrame &frame)
{
    Q_D(QAVSubtitleCodec);
    if (!d->avctx)
        return AVERROR(EINVAL);
    if (!d->gotOutput)
        return AVERROR(EAGAIN);
    *static_cast<QAVSubtitleFrame *>(&frame) = d->frame;
    d->gotOutput = 0;
    d->frame = {};
    return 0;
}

QT_END_NAMESPACE
