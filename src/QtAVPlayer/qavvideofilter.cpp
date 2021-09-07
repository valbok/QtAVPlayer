/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideofilter_p.h"
#include "qavfilter_p_p.h"
#include "qavcodec_p.h"
#include <QDebug>

extern "C" {
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

QT_BEGIN_NAMESPACE

class QAVVideoFilterPrivate : public QAVFilterPrivate
{
public:
    QAVVideoFilterPrivate(QAVFilter *q) : QAVFilterPrivate(q) { }

    int reconfigure(const QAVFrame &frame) override;

    int lastWidth = -1;
    int lastHeight = -1;
    enum AVPixelFormat lastFormat = AVPixelFormat(-2);
};

int QAVVideoFilterPrivate::reconfigure(const QAVFrame &frame)
{
    int ret = 0;
    const bool init = lastWidth != frame.frame()->width || lastHeight != frame.frame()->height || lastFormat != frame.frame()->format;
    if (!init)
        return ret;

    AVRational sample_aspect_ratio;
    sample_aspect_ratio.num = 0;
    sample_aspect_ratio.den = 1;
    AVRational time_base;
    time_base.num = 1;
    time_base.den = 600;
    auto codec = frame.codec();
    AVStream *stream = codec ? codec->stream() : nullptr;
    if (stream) {
        sample_aspect_ratio = stream->codecpar->sample_aspect_ratio;
        time_base = stream->time_base;
    }
    char buffersrc_args[256];
    snprintf(buffersrc_args, sizeof(buffersrc_args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             frame.frame()->width, frame.frame()->height, frame.frame()->format,
             time_base.num, time_base.den,
             sample_aspect_ratio.num, qMax(sample_aspect_ratio.den, 1));

    ret = avfilter_graph_create_filter(&src,
                                       avfilter_get_by_name("buffer"),
                                       "buffer", buffersrc_args, nullptr, graph);
    if (ret < 0)
        return ret;

    ret = avfilter_graph_create_filter(&sink,
                                       avfilter_get_by_name("buffersink"),
                                       "buffersink", nullptr, nullptr, graph);
    if (ret < 0)
        return ret;

    AVFilterInOut *outputs = nullptr;
    AVFilterInOut *inputs = nullptr;
    struct InOutDeleter {
        AVFilterInOut **io;
        ~InOutDeleter() { avfilter_inout_free(io); }
    } inDeleter{&inputs}, outDeleter{&outputs};

    if (!desc.isEmpty()) {
        outputs = avfilter_inout_alloc();
        inputs = avfilter_inout_alloc();

        outputs->name       = av_strdup("in");
        outputs->filter_ctx = src;
        outputs->pad_idx    = 0;
        outputs->next       = nullptr;

        inputs->name        = av_strdup("out");
        inputs->filter_ctx  = sink;
        inputs->pad_idx     = 0;
        inputs->next        = nullptr;
        
        ret = avfilter_graph_parse_ptr(graph, desc.toUtf8().constData(), &inputs, &outputs, nullptr);
        if (ret < 0)
            return ret;
    } else {
        ret = avfilter_link(src, 0, sink, 0);
        if (ret < 0)
            return ret;
    }

    ret = avfilter_graph_config(graph, nullptr);
    if (ret < 0)
        return ret;

    lastWidth = frame.frame()->width;
    lastHeight = frame.frame()->height;
    lastFormat = AVPixelFormat(frame.frame()->format);
    return ret;
}

QAVVideoFilter::QAVVideoFilter(const QString &desc, QObject *parent)
    : QAVFilter(*new QAVVideoFilterPrivate(this), desc, parent)
{
}

QT_END_NAMESPACE
