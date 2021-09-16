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

QAVFilter::QAVFilter(QAVFilterPrivate &d, QObject *parent)
    : QObject(parent)
    , d_ptr(&d)
{    
}

QAVFilter::~QAVFilter() = default;

bool QAVFilter::eof() const
{
    return !d_func()->sourceFrame;
}

QT_END_NAMESPACE
