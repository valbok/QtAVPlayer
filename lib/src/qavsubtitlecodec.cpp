/***************************************************************
 * Copyright (C) 2020, 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

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
    QAVPacket pkt;
    const int subtitle_out_max_size = 1024 * 1024;
};

QAVSubtitleCodec::QAVSubtitleCodec(const AVCodec *codec)
    : QAVCodec(*new QAVSubtitleCodecPrivate(this), codec)
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

int QAVSubtitleCodec::write(const QAVStreamFrame &frame)
{
    Q_D(QAVSubtitleCodec);
    if (!d->avctx)
        return AVERROR(EINVAL);
    d->pkt.setStream(frame.stream());
    auto pkt = d->pkt.packet();
    int ret = av_new_packet(pkt, d->subtitle_out_max_size);
    if (ret < 0)
        return AVERROR(ENOMEM);
    auto f = static_cast<const QAVSubtitleFrame *>(&frame);
    auto subtitle_out_size = avcodec_encode_subtitle(
        d->avctx,
        pkt->data,
        pkt->size,
        f->subtitle());
    if (subtitle_out_size < 0) {
        return subtitle_out_size;
    }
    av_shrink_packet(pkt, subtitle_out_size);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
    auto sub = f->subtitle();
    pkt->time_base = AV_TIME_BASE_Q;
    pkt->pts = sub->pts;
    pkt->duration = av_rescale_q(sub->end_display_time, AVRational{ 1, 1000 }, pkt->time_base);
    auto enc = frame.stream().codec()->avctx();
    if (enc->codec_id == AV_CODEC_ID_DVB_SUBTITLE) {
        if (frame.stream().index() == 0)
            pkt->pts += av_rescale_q(sub->start_display_time, AVRational{ 1, 1000 }, pkt->time_base);
        else
            pkt->pts += av_rescale_q(sub->end_display_time, AVRational{ 1, 1000 }, pkt->time_base);
    }
    pkt->dts = pkt->pts;
#endif
    return 0;
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

int QAVSubtitleCodec::read(QAVPacket &pkt)
{
    Q_D(QAVSubtitleCodec);
    if (!d->avctx)
        return AVERROR(EINVAL);
    if (!d->pkt)
        return AVERROR(EAGAIN);
    pkt = d->pkt;
    d->pkt = {};
    return 0;
}

QT_END_NAMESPACE
