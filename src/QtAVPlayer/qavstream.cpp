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
};

QAVStream::QAVStream(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVStreamPrivate(this))
{
}

QAVStream::QAVStream(int index, AVFormatContext *ctx, const QSharedPointer<QAVCodec> &codec, QObject *parent)
    : QAVStream(parent)
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

QMap<QString, QString> QAVStream::metadata() const
{
    QMap<QString, QString> result;
    auto s = stream();
    if (s != nullptr) {
        AVDictionaryEntry *tag = nullptr;
        while ((tag = av_dict_get(s->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
            result[QString::fromUtf8(tag->key)] = QString::fromUtf8(tag->value);
    }

    return result;
}

QSharedPointer<QAVCodec> QAVStream::codec() const
{
    Q_D(const QAVStream);
    return d->codec;
}

double QAVStream::duration() const
{
    auto s = stream();
    if (s == nullptr || s->duration == AV_NOPTS_VALUE)
        return 0.0;

    return s->duration * av_q2d(s->time_base);
}

int64_t QAVStream::framesCount() const
{
    Q_D(const QAVStream);
    auto s = stream();
    if (s == nullptr)
        return 0;

    auto frames = s->nb_frames;
    if (frames)
        return frames;

    auto dur = duration();
    if (dur == 0 && d->ctx->duration != AV_NOPTS_VALUE)
        dur = d->ctx->duration / AV_TIME_BASE;

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
#endif

QT_END_NAMESPACE
