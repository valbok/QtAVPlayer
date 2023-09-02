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
#ifdef QT_AVPLAYER_MULTIMEDIA
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAbstractVideoSurface>
#else
#include <QtMultimedia/private/qabstractvideobuffer_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#endif
#endif
#include <QDebug>

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include "libavutil/imgutils.h"
#include <libavutil/mastering_display_metadata.h>
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

QAVVideoFrame::QAVVideoFrame()
    : QAVFrame(*new QAVVideoFramePrivate(this))
{
}

QAVVideoFrame::QAVVideoFrame(const QAVFrame &other)
    : QAVVideoFrame()
{
    operator=(other);
}

QAVVideoFrame::QAVVideoFrame(const QAVVideoFrame &other)
    : QAVVideoFrame()
{
    operator=(other);
}

QAVVideoFrame::QAVVideoFrame(const QSize &size, AVPixelFormat fmt)
    : QAVVideoFrame()
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QVariant QAVVideoFrame::handle(QRhi *rhi) const
{
    Q_D(const QAVVideoFrame);
    return d->videoBuffer().handle(rhi);
}
#else
QVariant QAVVideoFrame::handle() const
{
    Q_D(const QAVVideoFrame);
    return d->videoBuffer().handle();
}
#endif

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
    if (mapData.format == AV_PIX_FMT_NONE) {
        qWarning() << __FUNCTION__ << "Could not map:" << formatName();
        return QAVVideoFrame();
    }
    auto ctx = sws_getContext(size().width(), size().height(), mapData.format,
                              size().width(), size().height(), fmt,
                              SWS_BICUBIC, NULL, NULL, NULL);
    if (ctx == nullptr) {
        qWarning() << __FUNCTION__ << ": Could not get sws context:" << formatName();
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

#ifdef QT_AVPLAYER_MULTIMEDIA
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
    PlanarVideoBuffer(const QAVVideoFrame &frame, QVideoFrameFormat::PixelFormat format
        , QVideoFrame::HandleType type = QVideoFrame::NoHandle)
        : QAbstractVideoBuffer(type)
        , m_frame(frame)
        , m_pixelFormat(format)
    {
    }

    quint64 textureHandle(int plane) const override
    {
        if (m_textures.isNull())
            const_cast<PlanarVideoBuffer *>(this)->m_textures = m_frame.handle(m_rhi);
        if (m_textures.canConvert<QList<QVariant>>()) {
            auto textures = m_textures.toList();
            auto r = plane < textures.size() ? textures[plane].toULongLong() : 0;
            return r;
        }
        return m_textures.toULongLong();
    }

    QVideoFrame::MapMode mapMode() const override { return m_mode; }
    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData res;
        if (m_mode != QVideoFrame::NotMapped || mode == QVideoFrame::NotMapped)
            return res;

        m_mode = mode;
        auto mapData = m_frame.map();
        auto *desc = QVideoTextureHelper::textureDescription(m_pixelFormat);
        res.nPlanes = desc->nplanes;
        for (int i = 0; i < res.nPlanes; ++i) {
            if (!mapData.bytesPerLine[i])
                break;

            res.data[i] = mapData.data[i];
            res.bytesPerLine[i] = mapData.bytesPerLine[i];
            // TODO: Reimplement heightForPlane
            res.size[i] = mapData.bytesPerLine[i] * desc->heightForPlane(m_frame.size().height(), i);
        }
        return res;
    }
    void unmap() override { m_mode = QVideoFrame::NotMapped; }

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    std::unique_ptr<QVideoFrameTextures> mapTextures(QRhi *rhi) override
    {
        m_rhi = rhi;
        if (m_textures.isNull())
            m_textures = m_frame.handle(m_rhi);
        return nullptr;
    }

    static QVideoFrameFormat::ColorSpace colorSpace(const AVFrame *frame)
    {
        switch (frame->colorspace) {
        default:
        case AVCOL_SPC_UNSPECIFIED:
        case AVCOL_SPC_RESERVED:
        case AVCOL_SPC_FCC:
        case AVCOL_SPC_SMPTE240M:
        case AVCOL_SPC_YCGCO:
        case AVCOL_SPC_SMPTE2085:
        case AVCOL_SPC_CHROMA_DERIVED_NCL:
        case AVCOL_SPC_CHROMA_DERIVED_CL:
        case AVCOL_SPC_ICTCP: // BT.2100 ICtCp
            return QVideoFrameFormat::ColorSpace_Undefined;
        case AVCOL_SPC_RGB:
            return QVideoFrameFormat::ColorSpace_AdobeRgb;
        case AVCOL_SPC_BT709:
            return QVideoFrameFormat::ColorSpace_BT709;
        case AVCOL_SPC_BT470BG: // BT601
        case AVCOL_SPC_SMPTE170M: // Also BT601
            return QVideoFrameFormat::ColorSpace_BT601;
        case AVCOL_SPC_BT2020_NCL: // Non constant luminence
        case AVCOL_SPC_BT2020_CL: // Constant luminence
            return QVideoFrameFormat::ColorSpace_BT2020;
        }
    }

    static QVideoFrameFormat::ColorTransfer colorTransfer(const AVFrame *frame)
    {
        switch (frame->color_trc) {
        case AVCOL_TRC_BT709:
        // The following three cases have transfer characteristics identical to BT709
        case AVCOL_TRC_BT1361_ECG:
        case AVCOL_TRC_BT2020_10:
        case AVCOL_TRC_BT2020_12:
        case AVCOL_TRC_SMPTE240M: // almost identical to bt709
            return QVideoFrameFormat::ColorTransfer_BT709;
        case AVCOL_TRC_GAMMA22:
        case AVCOL_TRC_SMPTE428: // No idea, let's hope for the best...
        case AVCOL_TRC_IEC61966_2_1: // sRGB, close enough to 2.2...
        case AVCOL_TRC_IEC61966_2_4: // not quite, but probably close enough
            return QVideoFrameFormat::ColorTransfer_Gamma22;
        case AVCOL_TRC_GAMMA28:
            return QVideoFrameFormat::ColorTransfer_Gamma28;
        case AVCOL_TRC_SMPTE170M:
            return QVideoFrameFormat::ColorTransfer_BT601;
        case AVCOL_TRC_LINEAR:
            return QVideoFrameFormat::ColorTransfer_Linear;
        case AVCOL_TRC_SMPTE2084:
            return QVideoFrameFormat::ColorTransfer_ST2084;
        case AVCOL_TRC_ARIB_STD_B67:
            return QVideoFrameFormat::ColorTransfer_STD_B67;
        default:
            break;
        }
        return QVideoFrameFormat::ColorTransfer_Unknown;
    }

    static QVideoFrameFormat::ColorRange colorRange(const AVFrame *frame)
    {
        switch (frame->color_range) {
        case AVCOL_RANGE_MPEG:
            return QVideoFrameFormat::ColorRange_Video;
        case AVCOL_RANGE_JPEG:
            return QVideoFrameFormat::ColorRange_Full;
        default:
            return QVideoFrameFormat::ColorRange_Unknown;
        }
    }

    static float maxNits(const AVFrame *frame)
    {
        float maxNits = -1;
        for (int i = 0; i < frame->nb_side_data; ++i) {
            AVFrameSideData *sd = frame->side_data[i];
            // TODO: Longer term we might want to also support HDR10+ dynamic metadata
            if (sd->type == AV_FRAME_DATA_MASTERING_DISPLAY_METADATA) {
                auto data = reinterpret_cast<AVMasteringDisplayMetadata *>(sd->data);
                auto b = data->max_luminance;
                auto maybeLum = b.den != 0 ? 10'000.0 * qreal(b.num) / qreal(b.den) : std::optional<qreal>{}; 
                if (maybeLum)
                    maxNits = float(maybeLum.value());
            }
        }
        return maxNits;
    }
#endif

private:
    QAVVideoFrame m_frame;
    QVideoFrameFormat::PixelFormat m_pixelFormat = QVideoFrameFormat::Format_Invalid;
    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    QVariant m_textures;
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
    QRhi *m_rhi = nullptr;
#endif
};

#endif // #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

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
        case AV_PIX_FMT_YUV444P:
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
        case AV_PIX_FMT_VIDEOTOOLBOX:
        case AV_PIX_FMT_NV12:
            format = VideoFrame::Format_NV12;
            break;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        case AV_PIX_FMT_MEDIACODEC:
            format = VideoFrame::Format_SamplerExternalOES;
            break;
#endif
        default:
            // TODO: Add more supported formats instead of converting
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
        case MTLTextureHandle:
        case D3D11Texture2DHandle:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            type = HandleType::RhiTextureHandle;
#endif
            break;
        default:
            break;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return QVideoFrame(new PlanarVideoBuffer(result, type), size(), format);
#else
    QVideoFrameFormat videoFormat(size(), format);
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    videoFormat.setColorSpace(PlanarVideoBuffer::colorSpace(frame()));
    videoFormat.setColorTransfer(PlanarVideoBuffer::colorTransfer(frame()));
    videoFormat.setColorRange(PlanarVideoBuffer::colorRange(frame()));
    videoFormat.setMaxLuminance(PlanarVideoBuffer::maxNits(frame()));
#endif
    return QVideoFrame(new PlanarVideoBuffer(result, format, type), videoFormat);
#endif
}
#endif // #ifdef QT_AVPLAYER_MULTIMEDIA

QT_END_NAMESPACE
