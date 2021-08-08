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

extern "C"
{
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include "libavutil/imgutils.h"
};

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

QAVVideoFrame::QAVVideoFrame(const QSize &size, AVPixelFormat fmt, QObject *parent)
    : QAVVideoFrame(parent)
{
    frame()->format = fmt;
    frame()->width = size.width();
    frame()->height = size.height();
    av_frame_get_buffer(frame(), 1);
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

AVPixelFormat QAVVideoFrame::format() const
{
    return static_cast<AVPixelFormat>(frame()->format);
}

QString QAVVideoFrame::formatName() const
{
    return QLatin1String(av_pix_fmt_desc_get(QAVVideoFrame::format())->name);
}

QAVVideoFrame QAVVideoFrame::convertTo(AVPixelFormat fmt) const
{
    if (fmt == frame()->format)
        return *this;

    auto ctx = sws_getContext(size().width(), size().height(), AVPixelFormat(frame()->format),
                              size().width(), size().height(), fmt,
                              SWS_BICUBIC, NULL, NULL, NULL);
    if (ctx == nullptr) {
        qWarning() << __FUNCTION__ << "Could not get sws context";
        return QAVVideoFrame();
    }

    int ret = sws_setColorspaceDetails(ctx, sws_getCoefficients(SWS_CS_ITU601),
                                       0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);
    if (ret == -1) {
        qWarning() << __FUNCTION__ << "Colorspace not support";
        return QAVVideoFrame();
    }

    QAVVideoFrame result(size(), fmt);
    result.d_ptr->codec = codec();
    sws_scale(ctx, frame()->data, frame()->linesize, 0, result.size().height(), result.frame()->data, result.frame()->linesize);
    sws_freeContext(ctx);

    return result;
}

QT_END_NAMESPACE
