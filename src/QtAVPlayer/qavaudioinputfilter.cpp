/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS 1
#endif
#include <inttypes.h>

#include "qavframe.h"
#include "qavaudioinputfilter_p.h"
#include "qavinoutfilter_p_p.h"
#include "qavdemuxer_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/bprint.h>
}

QT_BEGIN_NAMESPACE

class QAVAudioInputFilterPrivate : public QAVInOutFilterPrivate
{
public:
    QAVAudioInputFilterPrivate(QAVInOutFilter *q)
        : QAVInOutFilterPrivate(q)
    { }

    AVSampleFormat format = AV_SAMPLE_FMT_NONE;
    int sample_rate = 0;
    uint64_t channel_layout = 0;
    int channels = 0;
};

QAVAudioInputFilter::QAVAudioInputFilter(QObject *parent)
    : QAVInOutFilter(*new QAVAudioInputFilterPrivate(this), parent)
{
}

QAVAudioInputFilter::QAVAudioInputFilter(const QAVFrame &frame, QObject *parent)
    : QAVAudioInputFilter(parent)
{
    Q_D(QAVAudioInputFilter);
    const auto & frm = frame.frame();
    const auto & stream = frame.stream().stream();
    d->format = frm->format != AV_SAMPLE_FMT_NONE ? AVSampleFormat(frm->format) : AVSampleFormat(stream->codecpar->format);
    d->sample_rate = frm->sample_rate ? frm->sample_rate : stream->codecpar->sample_rate;
    d->channel_layout = frm->channel_layout ? frm->channel_layout : stream->codecpar->channel_layout;
    d->channels = frm->channels ? frm->channels : stream->codecpar->channels;
}

QAVAudioInputFilter::QAVAudioInputFilter(const QAVAudioInputFilter &other)
    : QAVAudioInputFilter()
{
    *this = other;
}

QAVAudioInputFilter::~QAVAudioInputFilter() = default;

QAVAudioInputFilter &QAVAudioInputFilter::operator=(const QAVAudioInputFilter &other)
{
    Q_D(QAVAudioInputFilter);
    QAVInOutFilter::operator=(other);
    d->format = other.d_func()->format;
    d->sample_rate = other.d_func()->sample_rate;
    d->channel_layout = other.d_func()->channel_layout;
    d->channels = other.d_func()->channels;
    return *this;
}

int QAVAudioInputFilter::configure(AVFilterGraph *graph, AVFilterInOut *in)
{
    Q_D(QAVAudioInputFilter);
    AVBPrint args;
    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
    av_bprintf(&args, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s",
             1, d->sample_rate,
             d->sample_rate,
             av_get_sample_fmt_name(AVSampleFormat(d->format)));
    if (d->channel_layout)
        av_bprintf(&args, ":channel_layout=0x%" PRIx64, d->channel_layout);
    else
        av_bprintf(&args, ":channels=%d", d->channels);

    char name[255];
    static int index = 0;
    snprintf(name, sizeof(name), "abuffer_%d", index++);

    int ret = avfilter_graph_create_filter(&d->ctx, 
                                           avfilter_get_by_name("abuffer"),
                                           name, args.str, nullptr, graph);
    if (ret < 0)
        return ret;

    return avfilter_link(d->ctx, 0, in->filter_ctx, in->pad_idx);
}

QT_END_NAMESPACE
