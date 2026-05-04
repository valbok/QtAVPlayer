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
    AVBufferRef *hw_frames_ctx = nullptr;
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
    d->hw_frames_ctx = frm->hw_frames_ctx;
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
    static int index = 0;
    char name[255];
    snprintf(name, sizeof(name), "buffer_%d", index++);
    d->ctx = avfilter_graph_alloc_filter(graph, avfilter_get_by_name("buffer"), name);
    if (!d->ctx)
        return AVERROR(ENOMEM);

    AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();
    if (!par)
        return AVERROR(ENOMEM);

    par->format              = d->format;
    par->time_base           = d->time_base;
    par->frame_rate          = d->frame_rate;
    par->width               = d->width;
    par->height              = d->height;
    par->sample_aspect_ratio = d->sample_aspect_ratio.den > 0 ?
                               d->sample_aspect_ratio : AVRational{ 0, 1 };
    par->hw_frames_ctx       = d->hw_frames_ctx;

    auto ret = av_buffersrc_parameters_set(d->ctx, par);
    av_freep(&par);
    if (ret < 0) {
        qWarning() << "av_buffersrc_parameters_set failed:" << ret;
        return ret;
    }

    ret = avfilter_init_dict(d->ctx, NULL);
    if (ret < 0) {
        qWarning() << "avfilter_init_dict failed:" << ret;
        return ret;
    }

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
