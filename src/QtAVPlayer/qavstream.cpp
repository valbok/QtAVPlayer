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
    std::shared_ptr<QAVDemuxer> demuxer;
};

QAVStream::QAVStream(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVStreamPrivate(this))
{
}

QAVStream::QAVStream(int index, const std::shared_ptr<QAVDemuxer> &demuxer, QObject *parent)
    : QAVStream(parent)
{
    d_ptr->index = index;
    d_ptr->demuxer = demuxer;
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
    d_ptr->demuxer = other.d_ptr->demuxer;
    return *this;
}

QAVStream::operator bool() const
{
    Q_D(const QAVStream);
    return bool(d->demuxer);
}

AVStream *QAVStream::stream() const
{
    Q_D(const QAVStream);
    return d->demuxer ? d->demuxer->stream(d->index) : nullptr;
}

int QAVStream::index() const
{
    return d_func()->index;
}

QMap<QString, QString> QAVStream::metadata() const
{
    QMap<QString, QString> result;
    AVDictionaryEntry *tag = nullptr;
    if (stream() != nullptr) {
        while ((tag = av_dict_get(stream()->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
            result[QLatin1String(tag->key)] = QLatin1String(tag->value);
    }

    return result;
}

QAVCodec *QAVStream::codec() const
{
    Q_D(const QAVStream);
    return d->demuxer ? d->demuxer->codec(d->index) : nullptr;
}

double QAVStream::duration() const
{
    if (!stream())
        return 0.0;

    return stream()->duration * av_q2d(stream()->time_base);
}

QT_END_NAMESPACE
