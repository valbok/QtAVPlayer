/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qavframe_p.h"
#include "qavframe_p_p.h"
#include "qavcodec_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

QAVFrame::QAVFrame(QObject *parent)
    : QAVFrame(nullptr, parent)
{
}

QAVFrame::QAVFrame(const QAVCodec *c, QObject *parent)
    : QAVFrame(*new QAVFramePrivate, parent)
{
    d_ptr->codec = c;
}

QAVFrame::QAVFrame(const QAVFrame &other)
    : QAVFrame()
{
    *this = other;
}

QAVFrame::QAVFrame(QAVFramePrivate &d, QObject *parent)
    : QObject(parent)
    , d_ptr(&d)
{
    d_ptr->frame = av_frame_alloc();
}

QAVFrame &QAVFrame::operator=(const QAVFrame &other)
{
    int64_t pts = d_ptr->frame->pts;
    av_frame_unref(d_ptr->frame);
    av_frame_ref(d_ptr->frame, other.d_ptr->frame);

    if (d_ptr->frame->pts < 0)
        d_ptr->frame->pts = pts;

    d_ptr->codec = other.d_ptr->codec;
    return *this;
}

QAVFrame::operator bool() const
{
    Q_D(const QAVFrame);
    return d->codec && d->frame && (d->frame->data[0] || d->frame->data[1] || d->frame->data[2] || d->frame->data[3]);
}

QAVFrame::~QAVFrame()
{
    Q_D(QAVFrame);
    av_frame_free(&d->frame);
}

AVFrame *QAVFrame::frame() const
{
    Q_D(const QAVFrame);
    return d->frame;
}

double QAVFrame::pts() const
{
    Q_D(const QAVFrame);
    if (!d->frame || !d->codec)
        return -1;

    return d->frame->pts * av_q2d(d->codec->stream()->time_base);
}

QT_END_NAMESPACE
