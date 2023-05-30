/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavfilter_p.h"
#include "qavfilter_p_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

QAVFilter::QAVFilter(
    const QAVStream &stream,
    const QString &name,
    QAVFilterPrivate &d)
    : d_ptr(&d)
{
    d.stream = stream;
    d.name = name;
}

QAVFilter::~QAVFilter() = default;

bool QAVFilter::isEmpty() const
{
    return d_func()->isEmpty;
}

QT_END_NAMESPACE
