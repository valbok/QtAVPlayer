/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVSTREAM_H
#define QAVSTREAM_H

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QMap>
#include <QSharedPointer>
#include <QObject>
#include <memory>

QT_BEGIN_NAMESPACE

struct AVStream;
class QAVCodec;
class QAVDemuxer;
class QAVStreamPrivate;
class Q_AVPLAYER_EXPORT QAVStream : public QObject
{
public:
    QAVStream(QObject *parent = nullptr);
    QAVStream(int index, const std::shared_ptr<QAVDemuxer> &demuxer = std::shared_ptr<QAVDemuxer>(), QObject *parent = nullptr);
    QAVStream(const QAVStream &other);
    ~QAVStream();
    QAVStream &operator=(const QAVStream &other);
    operator bool() const;

    int index() const;
    AVStream *stream() const;
    double duration() const;
    QMap<QString, QString> metadata() const;

    QAVCodec *codec() const;

private:
    QScopedPointer<QAVStreamPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVStream)
};

Q_DECLARE_METATYPE(QAVStream)

QT_END_NAMESPACE

#endif
