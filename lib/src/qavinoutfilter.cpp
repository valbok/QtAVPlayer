/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavinoutfilter_p.h"
#include "qavinoutfilter_p_p.h"
#include <QDebug>

extern "C" {
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

QT_BEGIN_NAMESPACE

QAVInOutFilter::QAVInOutFilter()
    : QAVInOutFilter(*new QAVInOutFilterPrivate(this))
{
}

QAVInOutFilter::QAVInOutFilter(QAVInOutFilterPrivate &d)
    : d_ptr(&d)
{
}

QAVInOutFilter::~QAVInOutFilter() = default;

QAVInOutFilter::QAVInOutFilter(const QAVInOutFilter &other)
    : QAVInOutFilter()
{
    *this = other;
}

QAVInOutFilter &QAVInOutFilter::operator=(const QAVInOutFilter &other)
{
    d_ptr->ctx = other.d_ptr->ctx;
    d_ptr->name = other.d_ptr->name;
    return *this;
}

int QAVInOutFilter::configure(AVFilterGraph *graph, AVFilterInOut *in)
{
    Q_D(QAVInOutFilter);
    Q_UNUSED(graph);
    if (in->name)
        d->name = QString::fromUtf8(in->name);
    return 0;
}

AVFilterContext *QAVInOutFilter::ctx() const
{
    Q_D(const QAVInOutFilter);
    return d->ctx;
}

QString QAVInOutFilter::name() const
{
    return d_func()->name;
}

QT_END_NAMESPACE
