/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavstream.h"
#include "qavdemuxer_p.h"
#include "qavcodec_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/display.h>
#include <libavutil/time.h>
#include <libavcodec/version.h>
}

QT_BEGIN_NAMESPACE

class QAVStreamPrivate
{
    Q_DECLARE_PUBLIC(QAVStream)
public:
    QAVStreamPrivate(QAVStream *q) : q_ptr(q) { }

    QAVStream *q_ptr = nullptr;
    int index = -1;
    AVFormatContext *ctx = nullptr;
    QSharedPointer<QAVCodec> codec;
    QMap<QString, QString> metadata;
};

QAVStream::QAVStream()
    : d_ptr(new QAVStreamPrivate(this))
{
}

QAVStream::QAVStream(int index, AVFormatContext *ctx, const QSharedPointer<QAVCodec> &codec)
    : QAVStream()
{
    d_ptr->index = index;
    d_ptr->ctx = ctx;
    d_ptr->codec = codec;
}

QAVStream::~QAVStream()
{
}

QAVStream::QAVStream(const QAVStream &other)
    : QAVStream()
{
    *this = other;
}

QAVStream &QAVStream::operator=(const QAVStream &other)
{
    d_ptr->index = other.d_ptr->index;
    d_ptr->ctx = other.d_ptr->ctx;
    d_ptr->codec = other.d_ptr->codec;
    return *this;
}

QAVStream::operator bool() const
{
    Q_D(const QAVStream);
    return d->ctx != nullptr && d->codec && d->index >= 0;
}

AVStream *QAVStream::stream() const
{
    Q_D(const QAVStream);
    return d->index >= 0 && d->index < static_cast<int>(d->ctx->nb_streams) ? d->ctx->streams[d->index] : nullptr;
}

int QAVStream::index() const
{
    return d_func()->index;
}

static int streamRotation(const AVStream *stream)
{
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(60, 29, 100)
    auto ptr = av_packet_side_data_get(stream->codecpar->coded_side_data,
                                       stream->codecpar->nb_coded_side_data,
                                       AV_PKT_DATA_DISPLAYMATRIX);
    auto sideData = ptr ? ptr->data : nullptr;
#elif LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(55, 18, 0)
    auto sideData = av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, nullptr);
#else
    auto cb = [](const auto &data) { return data.type == AV_PKT_DATA_DISPLAYMATRIX; };
    auto end = stream->side_data + stream->nb_side_data;
    auto ptr = std::find_if(stream->side_data, end, cb);
    auto sideData = ptr != end ? ptr->data : nullptr;
#endif
    if (!sideData)
        return 0;
    auto rotation = static_cast<int>(std::round(av_display_rotation_get(reinterpret_cast<const int32_t *>(sideData))));
    if (rotation % 90 != 0)
        return 0;
    return rotation > 0 ? -rotation % 360 + 360 : -rotation % 360;
}

static QMap<QString, QString> streamMetadata(const AVStream *stream)
{
    QMap<QString, QString> metadata;
    AVDictionaryEntry *tag = nullptr;
    while ((tag = av_dict_get(stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        metadata[QString::fromUtf8(tag->key)] = QString::fromUtf8(tag->value);
    if (!metadata.contains("rotate"))
        metadata["rotate"] = QString::number(streamRotation(stream));
    return metadata;
}

QMap<QString, QString> QAVStream::metadata() const
{
    Q_D(const QAVStream);
    if (!d->metadata.isEmpty())
        return d->metadata;
    auto s = stream();
    if (!s)
        return {};
    const_cast<QAVStreamPrivate *>(d)->metadata = streamMetadata(s);
    return d->metadata;
}

QSharedPointer<QAVCodec> QAVStream::codec() const
{
    Q_D(const QAVStream);
    return d->codec;
}

double QAVStream::duration() const
{
    Q_D(const QAVStream);
    auto s = stream();
    if (!s)
        return 0.0;

    double ret = 0.0;
    if (s->duration != AV_NOPTS_VALUE)
        ret = s->duration * av_q2d(s->time_base);
    if (!ret && d->ctx->duration != AV_NOPTS_VALUE)
        ret = d->ctx->duration / AV_TIME_BASE;
    return ret;
}

int64_t QAVStream::framesCount() const
{
    auto s = stream();
    if (s == nullptr)
        return 0;

    auto frames = s->nb_frames;
    if (frames)
        return frames;

    auto dur = duration();
    // If frame count is not known, estimating it
    if (s->avg_frame_rate.num && s->avg_frame_rate.den && dur)
        return dur * av_q2d(s->avg_frame_rate);

    const auto tb = s->time_base;
    if ((tb.num == 1 && tb.den >= 24 && tb.den <= 60) ||
        (tb.num == 1001 && tb.den >= 24000 && tb.den <= 60000))
    {
        return s->duration;
    }

    return 0;
}

double QAVStream::frameRate() const
{
    Q_D(const QAVStream);
    auto s = stream();
    if (s == nullptr)
        return 0.0;
    AVRational fr = av_guess_frame_rate(d->ctx, s, nullptr);
    return fr.num && fr.den ? av_q2d({fr.den, fr.num}) : 0.0;
}

QAVStream::Progress::Progress(double duration, qint64 frames, double fr)
    : m_duration(duration)
    , m_expectedFramesCount(frames)
    , m_expectedFrameRate(fr)
{
}

QAVStream::Progress::Progress(const Progress &other)
{
    *this = other;
}

QAVStream::Progress &QAVStream::Progress::operator=(const Progress &other)
{
    m_pts = other.m_pts;
    m_duration = other.m_duration;
    m_framesCount = other.m_framesCount;
    m_expectedFramesCount = other.m_expectedFramesCount;
    m_expectedFrameRate = other.m_expectedFrameRate;
    m_time = other.m_time;
    m_diffs = other.m_diffs;
    return *this;
}

double QAVStream::Progress::pts() const
{
    return m_pts;
}

double QAVStream::Progress::duration() const
{
    return m_duration;
}

qint64 QAVStream::Progress::framesCount() const
{
    return m_framesCount;
}

qint64 QAVStream::Progress::expectedFramesCount() const
{
    return m_expectedFramesCount;
}

double QAVStream::Progress::expectedFrameRate() const
{
    return m_expectedFrameRate;
}

double QAVStream::Progress::frameRate() const
{
    return m_framesCount ? m_diffs / 1000000.0 / static_cast<double>(m_framesCount) : 0.0;
}

unsigned QAVStream::Progress::fps() const
{
    double fr = frameRate();
    return fr ? static_cast<unsigned>(1 / fr) : 0;
}

void QAVStream::Progress::onFrameSent(double pts)
{
    m_pts = pts;
    qint64 cur = av_gettime_relative();
    if (m_framesCount++ > 0) {
        qint64 diff = cur - m_time;
        if (diff > 0)
            m_diffs += diff;
    }
    m_time = cur;
}

bool operator==(const QAVStream &lhs, const QAVStream &rhs)
{
    return lhs.index() == rhs.index();
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QAVStream &stream)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    return dbg << QString(QLatin1String("QAVStream(%1)" )).arg(stream.index()).toLatin1().constData();
}

QDebug operator<<(QDebug dbg, const QAVStream::Progress &p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    return dbg << QString(QLatin1String("Progress(%1/%2 pts, %3/%4 frames, %5/%6 frame rate, %7 fps)"))
        .arg(p.pts())
        .arg(p.duration())
        .arg(p.framesCount())
        .arg(p.expectedFramesCount())
        .arg(p.frameRate())
        .arg(p.expectedFrameRate())
        .arg(p.fps()).toLatin1().constData();
}
#endif

QT_END_NAMESPACE
