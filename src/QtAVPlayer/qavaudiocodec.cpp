/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiocodec_p.h"
#include "qavcodec_p_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

QAVAudioCodec::QAVAudioCodec(QObject *parent)
    : QAVCodec(parent)
{
}

QAVAudioFormat QAVAudioCodec::audioFormat() const
{
    Q_D(const QAVCodec);
    QAVAudioFormat format;
    if (!d->avctx)
        return format;

    auto fmt = AVSampleFormat(d->avctx->sample_fmt);
    if (fmt == AV_SAMPLE_FMT_U8)
        format.setSampleFormat(QAVAudioFormat::UInt8);
    else if (fmt == AV_SAMPLE_FMT_S16)
        format.setSampleFormat(QAVAudioFormat::Int16);
    else if (fmt == AV_SAMPLE_FMT_S32)
        format.setSampleFormat(QAVAudioFormat::Int32);
    else if (fmt == AV_SAMPLE_FMT_FLT)
        format.setSampleFormat(QAVAudioFormat::Float);

    format.setSampleRate(d->avctx->sample_rate);
    format.setChannelCount(d->avctx->channels);

    return format;
}

QT_END_NAMESPACE
