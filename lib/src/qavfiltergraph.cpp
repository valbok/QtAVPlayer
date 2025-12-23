/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavfiltergraph_p.h"
#include "qavcodec_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

QT_BEGIN_NAMESPACE

class QAVFilterGraphPrivate
{
    Q_DECLARE_PUBLIC(QAVFilterGraph)
public:
    QAVFilterGraphPrivate(QAVFilterGraph *q) : q_ptr(q) { }

    int read();

    QAVFilterGraph *q_ptr = nullptr;
    QString desc;
    AVFilterGraph *graph = nullptr;
    AVFilterInOut *outputs = nullptr;
    AVFilterInOut *inputs = nullptr;
    QList<QAVVideoInputFilter> videoInputFilters;
    QList<QAVVideoOutputFilter> videoOutputFilters;
    QList<QAVAudioInputFilter> audioInputFilters;
    QList<QAVAudioOutputFilter> audioOutputFilters;
    mutable QMutex mutex;
};

QAVFilterGraph::QAVFilterGraph()
    : d_ptr(new QAVFilterGraphPrivate(this))
{    
}

QAVFilterGraph::~QAVFilterGraph()
{
    Q_D(QAVFilterGraph);
    avfilter_graph_free(&d->graph);
    avfilter_inout_free(&d->inputs);
    avfilter_inout_free(&d->outputs);
}

int QAVFilterGraph::parse(const QString &desc)
{
    Q_D(QAVFilterGraph);
    d->desc = desc;
    avfilter_graph_free(&d->graph);
    avfilter_inout_free(&d->inputs);
    avfilter_inout_free(&d->outputs);
    d->graph = avfilter_graph_alloc();
    return avfilter_graph_parse2(d->graph, desc.toUtf8().constData(), &d->inputs, &d->outputs);
}

int QAVFilterGraph::apply(const QAVFrame &frame)
{
    Q_D(QAVFilterGraph);
    if (!frame.stream())
        return 0;
    const AVMediaType codec_type = frame.stream().stream()->codecpar->codec_type;
    switch (codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        d->videoInputFilters.clear();
        d->videoOutputFilters.clear();
        break;
    case AVMEDIA_TYPE_AUDIO:
        d->audioInputFilters.clear();
        d->audioOutputFilters.clear();
        break;
    default:
        qWarning() << "Could not apply frame: Unsupported codec type:" << codec_type;
        return AVERROR(EINVAL);
    }

    int ret = 0;
    int i = 0;
    AVFilterInOut *cur = nullptr;
    for (cur = d->inputs, i = 0; cur; cur = cur->next, ++i) {
        switch (avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx)) {
        case AVMEDIA_TYPE_VIDEO: {
            if (codec_type == AVMEDIA_TYPE_VIDEO) {
                QAVVideoInputFilter filter(frame);
                ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->videoInputFilters.push_back(filter);
            }
        } break;
        case AVMEDIA_TYPE_AUDIO: {
            if (codec_type == AVMEDIA_TYPE_AUDIO) {
                QAVAudioInputFilter filter(frame);
                ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->audioInputFilters.push_back(filter);
            }
        } break;
        default:
            return AVERROR(EINVAL);
        }
    }

    for (cur = d->outputs, i = 0; cur; cur = cur->next, ++i) {
        switch (avfilter_pad_get_type(cur->filter_ctx->output_pads, cur->pad_idx)) {
        case AVMEDIA_TYPE_VIDEO: {
            if (codec_type == AVMEDIA_TYPE_VIDEO) {
                QAVVideoOutputFilter filter;
                ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->videoOutputFilters.push_back(filter);
            }
        } break;
        case AVMEDIA_TYPE_AUDIO: {
            if (codec_type == AVMEDIA_TYPE_AUDIO) {
                QAVAudioOutputFilter filter;
                int ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->audioOutputFilters.push_back(filter);
            }
        } break;
        default:
            return AVERROR(EINVAL);
        }
    }

    return ret;
}

int QAVFilterGraph::config()
{
    Q_D(QAVFilterGraph);
    return avfilter_graph_config(d->graph, nullptr);
}

QString QAVFilterGraph::desc() const
{
    Q_D(const QAVFilterGraph);
    return d->desc;
}

QMutex &QAVFilterGraph::mutex()
{
    Q_D(QAVFilterGraph);
    return d->mutex;
}

AVFilterGraph *QAVFilterGraph::graph() const
{
    return d_func()->graph;
}

QList<QAVVideoInputFilter> QAVFilterGraph::videoInputFilters() const
{
    return d_func()->videoInputFilters;
}

QList<QAVVideoOutputFilter> QAVFilterGraph::videoOutputFilters() const
{
    return d_func()->videoOutputFilters;
}

QList<QAVAudioInputFilter> QAVFilterGraph::audioInputFilters() const
{
    return d_func()->audioInputFilters;
}

QList<QAVAudioOutputFilter> QAVFilterGraph::audioOutputFilters() const
{
    return d_func()->audioOutputFilters;
}

QT_END_NAMESPACE
