/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavstream.h"
#include "qavdemuxer_p.h"
#include "qavcodec_p.h"

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
    AVStream *stream = nullptr;
    QSharedPointer<QAVCodec> codec;
};

QAVStream::QAVStream(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVStreamPrivate(this))
{
}

QAVStream::QAVStream(int index, AVStream *stream, const QSharedPointer<QAVCodec> &codec, QObject *parent)
    : QAVStream(parent)
{
    d_ptr->index = index;
    d_ptr->stream = stream;
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
    d_ptr->stream = other.d_ptr->stream;
    d_ptr->codec = other.d_ptr->codec;
    return *this;
}

QAVStream::operator bool() const
{
    Q_D(const QAVStream);
    return d->stream != nullptr && d->codec && d->index >= 0;
}

AVStream *QAVStream::stream() const
{
    Q_D(const QAVStream);
    return d->stream;
}

int QAVStream::index() const
{
    return d_func()->index;
}

QMap<QString, QString> QAVStream::metadata() const
{
    Q_D(const QAVStream);
    QMap<QString, QString> result;
    AVDictionaryEntry *tag = nullptr;
    if (d->stream != nullptr) {
        while ((tag = av_dict_get(d->stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
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
    Q_D(const QAVStream);
    if (!d->stream)
        return 0.0;

    return d->stream->duration * av_q2d(d->stream->time_base);
}

QT_END_NAMESPACE
