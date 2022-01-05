/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideoinputfilter_p.h"
#include "qavinoutfilter_p_p.h"
#include "qavdemuxer_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/bprint.h>
}

QT_BEGIN_NAMESPACE

class QAVVideoInputFilterPrivate : public QAVInOutFilterPrivate
{
public:
    QAVVideoInputFilterPrivate(QAVInOutFilter *q, const QAVDemuxer *demuxer) : QAVInOutFilterPrivate(q), demuxer(demuxer) { }
    const QAVDemuxer *demuxer = nullptr;
};

QAVVideoInputFilter::QAVVideoInputFilter(QObject *parent)
    : QAVVideoInputFilter(nullptr, parent)
{
}

QAVVideoInputFilter::QAVVideoInputFilter(const QAVDemuxer *demuxer, QObject *parent)
    : QAVInOutFilter(*new QAVVideoInputFilterPrivate(this, demuxer), parent)
{
}

QAVVideoInputFilter::QAVVideoInputFilter(const QAVVideoInputFilter &other)
    : QAVVideoInputFilter()
{
    *this = other;
}

QAVVideoInputFilter::~QAVVideoInputFilter() = default;

QAVVideoInputFilter &QAVVideoInputFilter::operator=(const QAVVideoInputFilter &other)
{
    Q_D(QAVVideoInputFilter);
    QAVInOutFilter::operator=(other);
    d->demuxer = other.d_func()->demuxer;
    return *this;
}

int QAVVideoInputFilter::configure(AVFilterGraph *graph, AVFilterInOut *in)
{
    Q_D(QAVVideoInputFilter);
    AVStream *stream = d->demuxer ? d->demuxer->videoStream().stream() : nullptr;
    if (!stream)
        return AVERROR(EINVAL);

    AVRational sample_aspect_ratio = stream->codecpar->sample_aspect_ratio;
    AVRational time_base = stream->time_base;
    
    AVBPrint args;
    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
    av_bprintf(&args,
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:"
             "pixel_aspect=%d/%d",
             stream->codecpar->width, stream->codecpar->height, stream->codecpar->format,
             time_base.num, time_base.den,
             sample_aspect_ratio.num, qMax(sample_aspect_ratio.den, 1));
    auto fr = d->demuxer->frameRate();
    if (fr.num && fr.den)
        av_bprintf(&args, ":frame_rate=%d/%d", fr.num, fr.den);

    static int index = 0;
    char name[255];
    snprintf(name, sizeof(name), "buffer_%d", index++);

    int ret = avfilter_graph_create_filter(&d->ctx,
                                           avfilter_get_by_name("buffer"),
                                           name, args.str, nullptr, graph);
    if (ret < 0)
        return ret;
    
    ret = avfilter_link(d->ctx, 0, in->filter_ctx, in->pad_idx);
    if (ret < 0)
        return ret;

    return 0;
}

QT_END_NAMESPACE
