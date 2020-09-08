/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiocodec_p.h"
#include "qavcodec_p_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

QAVAudioCodec::QAVAudioCodec(QObject *parent)
    : QAVCodec(parent)
{
}

QAudioFormat QAVAudioCodec::audioFormat() const
{
    Q_D(const QAVCodec);
    QAudioFormat format;
    if (!d->avctx)
        return format;

    format.setSampleType(QAudioFormat::Unknown);
    auto fmt = AVSampleFormat(d->avctx->sample_fmt);
    if (fmt == AV_SAMPLE_FMT_U8) {
        format.setSampleSize(8);
        format.setSampleType(QAudioFormat::UnSignedInt);
    } else if (fmt == AV_SAMPLE_FMT_S16) {
        format.setSampleSize(16);
        format.setSampleType(QAudioFormat::SignedInt);
    } else if (fmt == AV_SAMPLE_FMT_S32) {
        format.setSampleSize(32);
        format.setSampleType(QAudioFormat::SignedInt);
    } else if (fmt == AV_SAMPLE_FMT_FLT) {
        format.setSampleSize(32);
        format.setSampleType(QAudioFormat::Float);
    }

    format.setCodec(QLatin1String("audio/pcm"));
    format.setSampleRate(d->avctx->sample_rate);
    format.setChannelCount(d->avctx->channels);
    format.setByteOrder(QAudioFormat::LittleEndian);

    return format;
}

QT_END_NAMESPACE
