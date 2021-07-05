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
#include <QSize>
#include <QDebug>

QT_BEGIN_NAMESPACE

static const QAVVideoCodec *videoCodec(const QAVCodec *c)
{
    return reinterpret_cast<const QAVVideoCodec *>(c);
}

class QAVVideoFramePrivate : public QAVFramePrivate
{
    Q_DECLARE_PUBLIC(QAVVideoFrame)
public:
    QAVVideoFramePrivate(QAVVideoFrame *q) : q_ptr(q) { }

    QAVVideoBuffer &videoBuffer() const
    {
        if (!buffer) {
            auto c = videoCodec(q_ptr->codec());
            auto buf = c && c->device() ? c->device()->videoBuffer(*q_ptr) : new QAVVideoBuffer_CPU(*q_ptr);
            const_cast<QAVVideoFramePrivate*>(this)->buffer.reset(buf);
        }

        return *buffer;
    }

    QAVVideoFrame *q_ptr = nullptr;
    QScopedPointer<QAVVideoBuffer> buffer;
};

QAVVideoFrame::QAVVideoFrame(QObject *parent)
    : QAVFrame(*new QAVVideoFramePrivate(this), parent)
{
    qRegisterMetaType<QAVVideoFrame>();
}

QAVVideoFrame::QAVVideoFrame(const QAVFrame &other, QObject *parent)
    : QAVVideoFrame(parent)
{
    operator=(other);
}

QAVVideoFrame::QAVVideoFrame(const QAVVideoFrame &other, QObject *parent)
    : QAVVideoFrame(parent)
{
    operator=(other);
}

QAVVideoFrame &QAVVideoFrame::operator=(const QAVFrame &other)
{
    Q_D(QAVVideoFrame);
    QAVFrame::operator=(other);
    d->buffer.reset();
    return *this;
}

QAVVideoFrame &QAVVideoFrame::operator=(const QAVVideoFrame &other)
{
    Q_D(QAVVideoFrame);
    QAVFrame::operator=(other);
    d->buffer.reset();
    return *this;
}

QSize QAVVideoFrame::size() const
{
    Q_D(const QAVFrame);
    return {d->frame->width, d->frame->height};
}

QAVVideoFrame::MapData QAVVideoFrame::map() const
{
    Q_D(const QAVVideoFrame);
    return d->videoBuffer().map();
}

QAVVideoFrame::HandleType QAVVideoFrame::handleType() const
{
    Q_D(const QAVVideoFrame);
    return d->videoBuffer().handleType();
}

QVariant QAVVideoFrame::handle() const
{
    Q_D(const QAVVideoFrame);
    return d->videoBuffer().handle();
}

QT_END_NAMESPACE
