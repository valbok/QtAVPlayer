/*********************************************************
 * Copyright (C) 2024, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qtavplayer/qavaudioconverter.h"
#include <QDebug>

extern "C" {
#include "libswresample/swresample.h"
}

QT_BEGIN_NAMESPACE

class QAVAudioConverterPrivate
{
public:
    SwrContext *swr_ctx = nullptr;
#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 23, 0)
    int64_t outChannelLayout = 0;
#else
    AVChannelLayout outChannelLayout;
#endif
    AVSampleFormat outFormat = AV_SAMPLE_FMT_NONE;
    int outSampleRate = 0;

    uint8_t *audioBuf = nullptr;
};

QAVAudioConverter::QAVAudioConverter()
    : d_ptr(new QAVAudioConverterPrivate)
{
}

QAVAudioConverter::~QAVAudioConverter()
{
    Q_D(QAVAudioConverter);
    swr_free(&d->swr_ctx);
    av_freep(&d->audioBuf);
}

QByteArray QAVAudioConverter::data(const QAVAudioFrame &audioFrame)
{
    Q_D(QAVAudioConverter);
    const auto frame = audioFrame.frame();
    if (!frame)
        return {};

    QByteArray audioData;
    const auto fmt = audioFrame.format();
    AVSampleFormat outFormat = AV_SAMPLE_FMT_NONE;
#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 23, 0)
    int64_t outChannelLayout = av_get_default_channel_layout(fmt.channelCount());
#else
    AVChannelLayout outChannelLayout;
    av_channel_layout_default(&outChannelLayout, fmt.channelCount());
#endif
    int outSampleRate = fmt.sampleRate();

    switch (fmt.sampleFormat()) {
    case QAVAudioFormat::UInt8:
        outFormat = AV_SAMPLE_FMT_U8;
        break;
    case QAVAudioFormat::Int16:
        outFormat = AV_SAMPLE_FMT_S16;
        break;
    case QAVAudioFormat::Int32:
        outFormat = AV_SAMPLE_FMT_S32;
        break;
    case QAVAudioFormat::Float:
        outFormat = AV_SAMPLE_FMT_FLT;
        break;
    default:
        qWarning() << "Could not negotiate output format:" << fmt.sampleFormat();
        return {};
    }

#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 23, 0)
    int64_t channelLayout = (frame->channel_layout && frame->channels == av_get_channel_layout_nb_channels(frame->channel_layout))
        ? frame->channel_layout
        : av_get_default_channel_layout(frame->channels);
    bool needsConvert = frame->format != outFormat || channelLayout != outChannelLayout || frame->sample_rate != d->outSampleRate;
#else
    AVChannelLayout channelLayout = frame->ch_layout;
    bool needsConvert = frame->format != outFormat || av_channel_layout_compare(&channelLayout, &outChannelLayout) || frame->sample_rate != outSampleRate;
#endif

    if (needsConvert) {
        bool needsCtxChange = outFormat != d->outFormat || outSampleRate != d->outSampleRate ||
#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 23, 0)
            outChannelLayout != d->outChannelLayout;
#else
            av_channel_layout_compare(&outChannelLayout, &d->outChannelLayout);
#endif
        if (needsCtxChange || !d->swr_ctx) {
            swr_free(&d->swr_ctx);
#if LIBSWRESAMPLE_VERSION_INT <= AV_VERSION_INT(4, 4, 0)
            d->swr_ctx = swr_alloc_set_opts(nullptr,
                                            outChannelLayout, outFormat, outSampleRate,
                                            channelLayout, AVSampleFormat(frame->format), frame->sample_rate,
                                            0, nullptr);
#else
            swr_alloc_set_opts2(&d->swr_ctx,
                                &outChannelLayout, outFormat, outSampleRate,
                                &channelLayout, AVSampleFormat(frame->format), frame->sample_rate,
                                0, nullptr);
#endif
            int ret = swr_init(d->swr_ctx);
            if (!d->swr_ctx || ret < 0) {
                qWarning() << "Could not init SwrContext:" << ret;
                return {};
            }
            d->outChannelLayout = outChannelLayout;
            d->outFormat = outFormat;
            d->outSampleRate = outSampleRate;
        }
    }

    if (d->swr_ctx) {
        const uint8_t **in = (const uint8_t **)frame->extended_data;
        int outCount = (int64_t)frame->nb_samples * outSampleRate / frame->sample_rate + 256;
        int outSize = av_samples_get_buffer_size(nullptr, fmt.channelCount(), outCount, outFormat, 0);

        av_freep(&d->audioBuf);
        uint8_t **out = &d->audioBuf;
        unsigned bufSize = 0;
        av_fast_malloc(&d->audioBuf, &bufSize, outSize);

        int samples = swr_convert(d->swr_ctx, out, outCount, in, frame->nb_samples);
        if (samples < 0) {
            qWarning() << "Could not convert audio samples";
            return {};
        }

        int size = samples * fmt.channelCount() * av_get_bytes_per_sample(outFormat);
        // Make deep copy
        audioData = QByteArray((const char *)d->audioBuf, size);
    } else {
        int size = av_samples_get_buffer_size(nullptr,
#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 23, 0)
                                              frame->channels,
#else
                                              outChannelLayout.nb_channels,
#endif
                                              frame->nb_samples,
                                              AVSampleFormat(frame->format), 1);
        // Return data from the frame
        audioData = QByteArray::fromRawData((const char *)frame->data[0], size);
    }

    return audioData;
}

QT_END_NAMESPACE
