/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudioframe.h"
#include "qavframe_p.h"
#include "qavaudiocodec_p.h"
#include <QDebug>

extern "C" {
#include "libswresample/swresample.h"
}

QT_BEGIN_NAMESPACE

class QAVAudioFramePrivate : public QAVFramePrivate
{
public:
    QAVAudioFormat outAudioFormat;
    int inSampleRate = 0;
    SwrContext *swr_ctx = nullptr;
    uint8_t *audioBuf = nullptr;
    QByteArray data;
};

QAVAudioFrame::QAVAudioFrame()
    : QAVFrame(*new QAVAudioFramePrivate)
{
}

QAVAudioFrame::~QAVAudioFrame()
{
    Q_D(QAVAudioFrame);
    swr_free(&d->swr_ctx);
    av_freep(&d->audioBuf);
}

QAVAudioFrame::QAVAudioFrame(const QAVFrame &other)
    : QAVFrame(*new QAVAudioFramePrivate)
{
    operator=(other);
}

QAVAudioFrame::QAVAudioFrame(const QAVAudioFrame &other)
    : QAVFrame(*new QAVAudioFramePrivate)
{
    operator=(other);
}

QAVAudioFrame::QAVAudioFrame(const QAVAudioFormat &format, const QByteArray &data)
    : QAVAudioFrame()
{
    Q_D(QAVAudioFrame);
    d->outAudioFormat = format;
    d->data = data;
}

QAVAudioFrame &QAVAudioFrame::operator=(const QAVFrame &other)
{
    Q_D(QAVAudioFrame);
    QAVFrame::operator=(other);
    d->data.clear();

    return *this;
}

QAVAudioFrame &QAVAudioFrame::operator=(const QAVAudioFrame &other)
{
    Q_D(QAVAudioFrame);
    QAVFrame::operator=(other);
    auto rhs = reinterpret_cast<QAVAudioFramePrivate *>(other.d_ptr.get());
    d->outAudioFormat = rhs->outAudioFormat;
    d->data = rhs->data;

    return *this;
}

QAVAudioFrame::operator bool() const
{
    Q_D(const QAVAudioFrame);
    return (d->outAudioFormat &&!d->data.isEmpty()) || QAVFrame::operator bool();
}

static const QAVAudioCodec *audioCodec(const QAVCodec *c)
{
    return reinterpret_cast<const QAVAudioCodec *>(c);
}

QAVAudioFormat QAVAudioFrame::format() const
{
    Q_D(const QAVAudioFrame);
    if (d->outAudioFormat)
        return d->outAudioFormat;

    if (!d->stream)
        return {};

    auto c = audioCodec(d->stream.codec().data());
    if (!c)
        return {};

    auto format = c->audioFormat();
    if (format.sampleFormat() != QAVAudioFormat::Int32)
        format.setSampleFormat(QAVAudioFormat::Int32);

    return format;
}

QByteArray QAVAudioFrame::data() const
{
    auto d = const_cast<QAVAudioFramePrivate *>(reinterpret_cast<QAVAudioFramePrivate *>(d_ptr.get()));
    const auto frame = d->frame;
    if (!frame)
        return {};

    if (d->outAudioFormat && !d->data.isEmpty())
        return d->data;

    const auto fmt = format();
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
    bool needsConvert = frame->format != outFormat || channelLayout != outChannelLayout || frame->sample_rate != outSampleRate;
#else
    AVChannelLayout channelLayout =frame->ch_layout;
    bool needsConvert = frame->format != outFormat || av_channel_layout_compare(&channelLayout, &outChannelLayout) || frame->sample_rate != outSampleRate;
#endif

    if (needsConvert && (fmt != d->outAudioFormat || frame->sample_rate != d->inSampleRate || !d->swr_ctx)) {
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
        d->data = QByteArray((const char *)d->audioBuf, size);
    } else {
        int size = av_samples_get_buffer_size(nullptr,
#if LIBAVUTIL_VERSION_INT <= AV_VERSION_INT(57, 23, 0)
                                              frame->channels,
#else
                                              outChannelLayout.nb_channels,
#endif
                                              frame->nb_samples,
                                              AVSampleFormat(frame->format), 1);
        d->data = QByteArray::fromRawData((const char *)frame->data[0], size);
    }

    d->inSampleRate = frame->sample_rate;
    d->outAudioFormat = fmt;
    return d->data;
}

QT_END_NAMESPACE
