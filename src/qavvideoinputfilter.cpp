/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavframe.h"
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
    QAVVideoInputFilterPrivate(QAVInOutFilter *q)
        : QAVInOutFilterPrivate(q)
    { }

    AVPixelFormat format = AV_PIX_FMT_NONE;
    int width = 0;
    int height = 0;
    AVRational sample_aspect_ratio{};
    AVRational time_base{};
    AVRational frame_rate{};
};

QAVVideoInputFilter::QAVVideoInputFilter()
    : QAVInOutFilter(*new QAVVideoInputFilterPrivate(this))
{
}

QAVVideoInputFilter::QAVVideoInputFilter(const QAVFrame &frame)
    : QAVVideoInputFilter()
{
    Q_D(QAVVideoInputFilter);
    const auto & frm = frame.frame();
    const auto & stream = frame.stream().stream();
    d->format =  frm->format != AV_PIX_FMT_NONE ? AVPixelFormat(frm->format) : AVPixelFormat(stream->codecpar->format);
    d->width = frm->width ? frm->width : stream->codecpar->width;
    d->height = frm->height ? frm->height : stream->codecpar->height;
    d->sample_aspect_ratio = frm->sample_aspect_ratio.num && frm->sample_aspect_ratio.den ? frm->sample_aspect_ratio : stream->codecpar->sample_aspect_ratio;
    d->time_base = stream->time_base;
    d->frame_rate = stream->avg_frame_rate;
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
    d->format = other.d_func()->format;
    d->width = other.d_func()->width;
    d->height = other.d_func()->height;
    d->sample_aspect_ratio = other.d_func()->sample_aspect_ratio;
    d->time_base = other.d_func()->time_base;
    d->frame_rate = other.d_func()->frame_rate;
    return *this;
}

int QAVVideoInputFilter::configure(AVFilterGraph *graph, AVFilterInOut *in)
{
    QAVInOutFilter::configure(graph, in);
    Q_D(QAVVideoInputFilter);
    AVBPrint args;
    av_bprint_init(&args, 0, AV_BPRINT_SIZE_AUTOMATIC);
    av_bprintf(&args,
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:"
             "pixel_aspect=%d/%d",
             d->width, d->height, d->format,
             d->time_base.num, d->time_base.den,
             d->sample_aspect_ratio.num, qMax(d->sample_aspect_ratio.den, 1));
    if (d->frame_rate.num && d->frame_rate.den)
        av_bprintf(&args, ":frame_rate=%d/%d", d->frame_rate.num, d->frame_rate.den);

    static int index = 0;
    char name[255];
    snprintf(name, sizeof(name), "buffer_%d", index++);

    int ret = avfilter_graph_create_filter(&d->ctx,
                                           avfilter_get_by_name("buffer"),
                                           name, args.str, nullptr, graph);
    if (ret < 0)
        return ret;

    return avfilter_link(d->ctx, 0, in->filter_ctx, in->pad_idx);
}

bool QAVVideoInputFilter::supports(const QAVFrame &frame) const
{
    Q_D(const QAVVideoInputFilter);
    if (!frame)
        return true;
    const auto & frm = frame.frame();
    return d->width == frm->width
           && d->height == frm->height
           && d->format == frm->format;
}

QT_END_NAMESPACE
