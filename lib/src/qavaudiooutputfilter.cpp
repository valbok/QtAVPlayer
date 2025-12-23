/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiooutputfilter_p.h"
#include "qavinoutfilter_p_p.h"
#include <QDebug>

extern "C" {
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/bprint.h>
}

QT_BEGIN_NAMESPACE

QAVAudioOutputFilter::QAVAudioOutputFilter()
    : QAVInOutFilter(*new QAVInOutFilterPrivate(this))
{
}

QAVAudioOutputFilter::~QAVAudioOutputFilter() = default;

int QAVAudioOutputFilter::configure(AVFilterGraph *graph, AVFilterInOut *out)
{
    QAVInOutFilter::configure(graph, out);
    Q_D(QAVInOutFilter);
    char name[255];
    static int index = 0;
    snprintf(name, sizeof(name), "out_%d", index++);
    int ret = avfilter_graph_create_filter(&d->ctx, 
                                           avfilter_get_by_name("abuffersink"),
                                           name, nullptr, nullptr, graph);
    if (ret < 0)
        return ret;

    return avfilter_link(out->filter_ctx, out->pad_idx, d->ctx, 0);
}

QT_END_NAMESPACE
