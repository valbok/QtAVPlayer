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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAbstractVideoSurface>
#endif
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class PlanarVideoBuffer : public QAbstractPlanarVideoBuffer
{
public:
    PlanarVideoBuffer(const QAVVideoFrame &frame, HandleType type = NoHandle)
        : QAbstractPlanarVideoBuffer(type), m_frame(frame)
    {
    }

    MapMode mapMode() const override { return m_mode; }
    int map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]) override
    {
        if (m_mode != NotMapped || mode == NotMapped)
            return 0;

        auto mapData = m_frame.map();
        m_mode = mode;
        if (numBytes)
            *numBytes = mapData.size;

        int i = 0;
        for (; i < 4; ++i) {
            if (!mapData.bytesPerLine[i])
                break;

            bytesPerLine[i] = mapData.bytesPerLine[i];
            data[i] = mapData.data[i];
        }

        return i;
    }
    void unmap() override { m_mode = NotMapped; }

private:
    QAVVideoFrame m_frame;
    MapMode m_mode = NotMapped;
};

QAVVideoFrame::operator QVideoFrame() const
{
    QAVVideoFrame result = *this;
    if (!result)
        return QVideoFrame();

    QVideoFrame::PixelFormat format = QVideoFrame::Format_Invalid;
    switch (frame()->format) {
        case AV_PIX_FMT_YUV420P:
            format = QVideoFrame::Format_YUV420P;
            break;
        case AV_PIX_FMT_YUV422P:
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            result = convertTo(AV_PIX_FMT_YUV420P);
            format = QVideoFrame::Format_YUV420P;
#else
            format = QVideoFrame::Format_YUV422P;
#endif
            break;
        case AV_PIX_FMT_YUV411P:
            result = convertTo(AV_PIX_FMT_YUV420P);
            format = QVideoFrame::Format_YUV420P;
            break;
        case AV_PIX_FMT_NV12:
            format = QVideoFrame::Format_NV12;
            break;
        case AV_PIX_FMT_D3D11:
            format = QVideoFrame::Format_NV12;
            break;
        default:
            qDebug() << "Format not supported: " << result.formatName();
    }

    return QVideoFrame(new PlanarVideoBuffer(result), size(), format);
}
#endif

QT_END_NAMESPACE
