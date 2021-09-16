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

QAVInOutFilter::QAVInOutFilter(QObject *parent)
    : QAVInOutFilter(*new QAVInOutFilterPrivate(this), parent)
{
}

QAVInOutFilter::QAVInOutFilter(QAVInOutFilterPrivate &d, QObject *parent)
    : QObject(parent)
    , d_ptr(&d)
{
}

QAVInOutFilter::~QAVInOutFilter() = default;

QAVInOutFilter::QAVInOutFilter(const QAVInOutFilter &other)
    : QAVInOutFilter(nullptr)
{
    *this = other;
}

QAVInOutFilter &QAVInOutFilter::operator=(const QAVInOutFilter &other)
{
    d_ptr->ctx = other.d_ptr->ctx;
    return *this;
}

AVFilterContext *QAVInOutFilter::ctx() const
{
    Q_D(const QAVInOutFilter);
    return d->ctx;
}

QT_END_NAMESPACE
