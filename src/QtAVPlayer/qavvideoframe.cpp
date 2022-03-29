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
#else
#include <QtMultimedia/private/qabstractvideobuffer_p.h>
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
            auto c = videoCodec(stream.codec().data());
            auto buf = c && c->device() && frame->format == c->device()->format() ? c->device()->videoBuffer(*q_ptr) : new QAVVideoBuffer_CPU(*q_ptr);
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

    auto mapData = map();
    auto ctx = sws_getContext(size().width(), size().height(), mapData.format,
                              size().width(), size().height(), fmt,
                              SWS_BICUBIC, NULL, NULL, NULL);
    if (ctx == nullptr) {
        qWarning() << __FUNCTION__ << ": Could not get sws context:" << av_pix_fmt_desc_get(AVPixelFormat(frame()->format))->name;
        return QAVVideoFrame();
    }

    int ret = sws_setColorspaceDetails(ctx, sws_getCoefficients(SWS_CS_ITU601),
                                       0, sws_getCoefficients(SWS_CS_ITU709), 0, 0, 1 << 16, 1 << 16);
    if (ret == -1) {
        qWarning() << __FUNCTION__ << "Colorspace not support";
        return QAVVideoFrame();
    }

    QAVVideoFrame result(size(), fmt);
    result.d_ptr->stream = d_ptr->stream;
    sws_scale(ctx, mapData.data, mapData.bytesPerLine, 0, result.size().height(), result.frame()->data, result.frame()->linesize);
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

    QVariant handle() const override
    {
        return m_frame.handle();
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
#else
class PlanarVideoBuffer : public QAbstractVideoBuffer
{
public:
    PlanarVideoBuffer(const QAVVideoFrame &frame, QVideoFrame::HandleType type = QVideoFrame::NoHandle)
        : QAbstractVideoBuffer(type), m_frame(frame)
    {
    }

    quint64 textureHandle(int /*plane*/) const override
    {
        return m_frame.handle().toULongLong();
    }

    QVideoFrame::MapMode mapMode() const override { return m_mode; }
    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData res;
        if (m_mode != QVideoFrame::NotMapped || mode == QVideoFrame::NotMapped)
            return res;

        m_mode = mode;
        auto mapData = m_frame.map();
        int nPlanes = 0;
        for (;nPlanes < 4; ++nPlanes) {
            if (!mapData.bytesPerLine[nPlanes])
                break;

            res.bytesPerLine[nPlanes] = mapData.bytesPerLine[nPlanes];
            res.data[nPlanes] = mapData.data[nPlanes];
            // TODO: Check if size can be different
            res.size[nPlanes] = mapData.bytesPerLine[nPlanes];
        }

        res.nPlanes = nPlanes;
        return res;
    }
    void unmap() override { m_mode = QVideoFrame::NotMapped; }

private:
    QAVVideoFrame m_frame;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
};

#endif

QAVVideoFrame::operator QVideoFrame() const
{
    QAVVideoFrame result = *this;
    if (!result)
        return QVideoFrame();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using VideoFrame = QVideoFrame;
#else
    using VideoFrame = QVideoFrameFormat;
#endif

    VideoFrame::PixelFormat format = VideoFrame::Format_Invalid;
    switch (frame()->format) {
        case AV_PIX_FMT_RGB32:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            format = VideoFrame::Format_RGB32;
#else
            format = QVideoFrameFormat::Format_BGRA8888;
#endif
            break;
        case AV_PIX_FMT_YUV420P:
            format = VideoFrame::Format_YUV420P;
            break;
        case AV_PIX_FMT_YUV422P:
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
            result = convertTo(AV_PIX_FMT_YUV420P);
            format = VideoFrame::Format_YUV420P;
#else
            format = VideoFrame::Format_YUV422P;
#endif
            break;
        case AV_PIX_FMT_VAAPI:
        case AV_PIX_FMT_VDPAU:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            format = VideoFrame::Format_BGRA32;
#else
            format = QVideoFrameFormat::Format_RGBA8888;
#endif
            break;
        case AV_PIX_FMT_D3D11:
        case AV_PIX_FMT_NV12:
            format = VideoFrame::Format_NV12;
            break;
        default:
            result = convertTo(AV_PIX_FMT_YUV420P);
            format = VideoFrame::Format_YUV420P;
            break;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using HandleType = QAbstractVideoBuffer::HandleType;
#else
    using HandleType = QVideoFrame::HandleType;
#endif

    HandleType type = HandleType::NoHandle;
    switch (handleType()) {
        case GLTextureHandle:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            type = HandleType::GLTextureHandle;
#else
            type = HandleType::RhiTextureHandle;
#endif
            break;
        default:
            break;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return QVideoFrame(new PlanarVideoBuffer(result, type), size(), format);
#else
    return QVideoFrame(new PlanarVideoBuffer(result, type), {size(), format});
#endif
}

QT_END_NAMESPACE
