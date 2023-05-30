/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavcodec_p.h"
#include "qavcodec_p_p.h"

#include <QDebug>

extern "C" {
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

QAVCodec::QAVCodec()
    : QAVCodec(*new QAVCodecPrivate)
{
}

QAVCodec::QAVCodec(QAVCodecPrivate &d)
    : d_ptr(&d)
{
    d_ptr->avctx = avcodec_alloc_context3(nullptr);
}

QAVCodec::~QAVCodec()
{
    Q_D(QAVCodec);
    if (d->avctx)
        avcodec_free_context(&d->avctx);
}

void QAVCodec::setCodec(const AVCodec *c)
{
    d_func()->codec = c;
}

bool QAVCodec::open(AVStream *stream)
{
    Q_D(QAVCodec);

    if (!stream)
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

    av_opt_set_int(d->avctx, "refcounted_frames", true, 0);
    av_opt_set_int(d->avctx, "threads", 1, 0);
    ret = avcodec_open2(d->avctx, d->codec, nullptr);
    if (ret < 0) {
        qWarning() << "Could not open the codec:" << d->codec->name << ret;
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

const AVCodec *QAVCodec::codec() const
{
    return d_func()->codec;
}

void QAVCodec::flushBuffers()
{
     Q_D(QAVCodec);
     if (!d->avctx)
        return;
    avcodec_flush_buffers(d->avctx);
}
QT_END_NAMESPACE
