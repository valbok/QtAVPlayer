/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudioinputfilter_p.h"
#include "qavinoutfilter_p_p.h"
#include "qavdemuxer_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/bprint.h>
}

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

QT_BEGIN_NAMESPACE

class QAVAudioInputFilterPrivate : public QAVInOutFilterPrivate
{
public:
    QAVAudioInputFilterPrivate(QAVInOutFilter *q, const QAVDemuxer *demuxer) : QAVInOutFilterPrivate(q), demuxer(demuxer) { }
    const QAVDemuxer *demuxer = nullptr;
};

QAVAudioInputFilter::QAVAudioInputFilter(QObject *parent)
    : QAVAudioInputFilter(nullptr, parent)
{
}

QAVAudioInputFilter::QAVAudioInputFilter(const QAVDemuxer *demuxer, QObject *parent)
    : QAVInOutFilter(*new QAVAudioInputFilterPrivate(this, demuxer), parent)
{
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
    d->demuxer = other.d_func()->demuxer;
    return *this;
}

int QAVAudioInputFilter::configure(AVFilterGraph *graph, AVFilterInOut *in)
{
    Q_D(QAVAudioInputFilter);
    AVStream *stream = d->demuxer ? d->demuxer->audioStream() : nullptr;
    if (!stream)
        return AVERROR(EINVAL);
    
    AVBPrint args;
    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
    av_bprintf(&args, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s",
             1, stream->codecpar->sample_rate,
             stream->codecpar->sample_rate,
             av_get_sample_fmt_name(AVSampleFormat(stream->codecpar->format)));
    if (stream->codecpar->channel_layout)
        av_bprintf(&args, ":channel_layout=0x%" PRIx64, stream->codecpar->channel_layout);
    else
        av_bprintf(&args, ":channels=%d", stream->codecpar->channels);

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
