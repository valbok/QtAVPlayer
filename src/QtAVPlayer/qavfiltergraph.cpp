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
#include "qavdemuxer_p.h"
#include <QDebug>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

QT_BEGIN_NAMESPACE

class QAVFilterGraphPrivate
{
    Q_DECLARE_PUBLIC(QAVFilterGraph)
public:
    QAVFilterGraphPrivate(QAVFilterGraph *q, const QAVDemuxer &d) : q_ptr(q), demuxer(d) { }

    int read();

    QAVFilterGraph *q_ptr = nullptr;
    const QAVDemuxer &demuxer;
    QString desc;
    AVFilterGraph *graph = nullptr;    
    QList<QAVVideoInputFilter> videoInputFilters;
    QList<QAVVideoOutputFilter> videoOutputFilters;
    QList<QAVAudioInputFilter> audioInputFilters;
    QList<QAVAudioOutputFilter> audioOutputFilters;
};

QAVFilterGraph::QAVFilterGraph(const QAVDemuxer &demuxer, QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVFilterGraphPrivate(this, demuxer))
{    
}

QAVFilterGraph::~QAVFilterGraph()
{
    Q_D(QAVFilterGraph);
    avfilter_graph_free(&d->graph);
}

int QAVFilterGraph::parse(const QString &desc)
{
    Q_D(QAVFilterGraph);
    d->desc = desc;
    AVFilterInOut *outputs = nullptr;
    AVFilterInOut *inputs = nullptr;
    struct InOutDeleter {
        AVFilterInOut **io = nullptr;
        InOutDeleter(AVFilterInOut **o) : io(o) { }
        ~InOutDeleter() { avfilter_inout_free(io); }
    } inDeleter{ &inputs }, outDeleter{ &outputs };

    avfilter_graph_free(&d->graph);
    d->graph = avfilter_graph_alloc();
    int ret = avfilter_graph_parse2(d->graph, desc.toUtf8().constData(), &inputs, &outputs);
    if (ret < 0)
        return ret;

    d->videoInputFilters.clear();
    d->audioInputFilters.clear();
    int i = 0;
    AVFilterInOut *cur = nullptr;
    for (cur = inputs, i = 0; cur; cur = cur->next, ++i) {
        switch (avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx)) {
            case AVMEDIA_TYPE_VIDEO: {
                QAVVideoInputFilter filter(&d->demuxer);
                ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->videoInputFilters.push_back(filter);
            } break;

            case AVMEDIA_TYPE_AUDIO: {
                QAVAudioInputFilter filter(&d->demuxer);
                int ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->audioInputFilters.push_back(filter);
            } break;

            default:
                return AVERROR(EINVAL);
        }
    }

    d->videoOutputFilters.clear();
    d->audioOutputFilters.clear();
    for (cur = outputs, i = 0; cur; cur = cur->next, ++i) {
        switch (avfilter_pad_get_type(cur->filter_ctx->input_pads, cur->pad_idx)) {
            case AVMEDIA_TYPE_VIDEO: {
                QAVVideoOutputFilter filter;
                ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->videoOutputFilters.push_back(filter);
            } break;

            case AVMEDIA_TYPE_AUDIO: {
                QAVAudioOutputFilter filter;
                int ret = filter.configure(d->graph, cur);
                if (ret < 0)
                    return ret;
                d->audioOutputFilters.push_back(filter);
            } break;

            default:
                return AVERROR(EINVAL);
        }
    }

    return avfilter_graph_config(d->graph, nullptr);
}

QString QAVFilterGraph::desc() const
{
    Q_D(const QAVFilterGraph);
    return d->desc;
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
