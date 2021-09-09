/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavfilter_p.h"
#include "qavfilter_p_p.h"
#include <QDebug>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

QT_BEGIN_NAMESPACE

QAVFilter::QAVFilter(QAVFilterPrivate &d, const QString &desc, QObject *parent)
    : QObject(parent)
    , d_ptr(&d)
{
    d_ptr->graph = avfilter_graph_alloc();
    d_ptr->desc = desc;
}

QAVFilter::~QAVFilter()
{
    Q_D(QAVFilter);
    avfilter_graph_free(&d->graph);
}

QString QAVFilter::desc() const
{
    return d_func()->desc;
}

int QAVFilter::write(const QAVFrame &frame)
{
    Q_D(QAVFilter);
    int ret = d->reconfigure(frame);
    if (ret < 0)
        return ret;

    d->sourceFrame = frame;
    return av_buffersrc_add_frame(d->src, frame.frame());
}

QAVFrame QAVFilter::read()
{
    Q_D(QAVFilter);
    if (!d->sourceFrame)
        return {};

    auto frame = d->sourceFrame;
    int ret = av_buffersink_get_frame_flags(d->sink, frame.frame(), 0);
    if (ret < 0) {
        d->sourceFrame = QAVFrame();
        return {};
    }

    if (!frame.frame()->pkt_duration)
        frame.frame()->pkt_duration = d->sourceFrame.frame()->pkt_duration;
    frame.setFrameRate(av_buffersink_get_frame_rate(d->sink));
    frame.setTimeBase(av_buffersink_get_time_base(d->sink));
    return frame;
}

bool QAVFilter::eof() const
{
    return !d_func()->sourceFrame;
}

QT_END_NAMESPACE
