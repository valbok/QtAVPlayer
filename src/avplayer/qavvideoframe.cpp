/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideoframe.h"
#include "qavvideobuffer_cpu_p.h"
#include "qavframe_p.h"
#include "qavvideocodec_p.h"
#include "qavhwdevice_p.h"
#include <QVideoFrame>
#include <QDebug>

QT_BEGIN_NAMESPACE

QAVVideoFrame::QAVVideoFrame(QObject *parent)
    : QAVFrame(parent)
{
}

static const QAVVideoCodec *videoCodec(const QAVCodec *c)
{
    return reinterpret_cast<const QAVVideoCodec *>(c);
}

QAVVideoFrame::QAVVideoFrame(const QAVFrame &other, QObject *parent)
    : QAVVideoFrame(parent)
{
    operator=(other);
}

QAVVideoFrame &QAVVideoFrame::operator=(const QAVFrame &other)
{
    QAVFrame::operator=(other);
    return *this;
}

QSize QAVVideoFrame::size() const
{
    Q_D(const QAVFrame);
    return {d->frame->width, d->frame->height};
}

QAVVideoFrame::MapData QAVVideoFrame::map() const
{
    auto c = videoCodec(codec());
    if (!c)
        return {};

    if (c->device())
        return c->device()->map(*this);

    return QAVVideoBuffer_CPU(*this).map();
}

QAVVideoFrame::HandleType QAVVideoFrame::handleType() const
{
    auto c = videoCodec(codec());
    if (c && c->device())
        return c->device()->handleType();

    return QAVVideoFrame::NoHandle;
}

QVariant QAVVideoFrame::handle() const
{
    auto c = videoCodec(codec());
    if (c && c->device())
        return c->device()->handle(*this);

    return {};
}

QT_END_NAMESPACE
