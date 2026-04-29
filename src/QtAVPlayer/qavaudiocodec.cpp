/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavaudiocodec_p.h"
#include "qavcodec_p_p.h"
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

QT_BEGIN_NAMESPACE

QAVAudioCodec::QAVAudioCodec(const AVCodec *codec)
{
    setCodec(codec);
}

QAVAudioFormat QAVAudioCodec::audioFormat() const
{
    Q_D(const QAVCodec);
    QAVAudioFormat format;
    if (!d->avctx)
        return format;

    auto fmt = AVSampleFormat(d->avctx->sample_fmt);
    switch (fmt) {
    case AV_SAMPLE_FMT_U8:
        format.setSampleFormat(QAVAudioFormat::UInt8);
        break;
    case AV_SAMPLE_FMT_S16:
        format.setSampleFormat(QAVAudioFormat::Int16);
        break;
    case AV_SAMPLE_FMT_S32:
        format.setSampleFormat(QAVAudioFormat::Int32);
        break;
    case AV_SAMPLE_FMT_FLTP:
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_DBL:
    case AV_SAMPLE_FMT_DBLP:
        format.setSampleFormat(QAVAudioFormat::Float);
        break;
    default:
        break;
    }

    format.setSampleRate(d->avctx->sample_rate);
#if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(59, 23, 0)
    format.setChannelCount(d->avctx->channels);
#else
    format.setChannelCount(d->avctx->ch_layout.nb_channels);
#endif

    return format;
}

QT_END_NAMESPACE
