/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideooutputfilter_p.h"
#include "qavinoutfilter_p_p.h"
#include <QDebug>

extern "C" {
#include <libavfilter/buffersink.h>
}

QT_BEGIN_NAMESPACE

QAVVideoOutputFilter::QAVVideoOutputFilter()
    : QAVInOutFilter(*new QAVInOutFilterPrivate(this))
{
}

QAVVideoOutputFilter::~QAVVideoOutputFilter() = default;

int QAVVideoOutputFilter::configure(AVFilterGraph *graph, AVFilterInOut *out)
{
    QAVInOutFilter::configure(graph, out);
    Q_D(QAVInOutFilter);
    static int index = 0;
    char name[255];
    snprintf(name, sizeof(name), "buffersink_%d", index++);

    int ret = avfilter_graph_create_filter(&d->ctx, 
                                           avfilter_get_by_name("buffersink"),
                                           name, nullptr, nullptr, graph);
    if (ret < 0)
        return ret;

    return avfilter_link(out->filter_ctx, out->pad_idx, d->ctx, 0);
}

QT_END_NAMESPACE
