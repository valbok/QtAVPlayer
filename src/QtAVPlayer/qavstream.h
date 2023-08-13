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
#include <memory>

QT_BEGIN_NAMESPACE

struct AVStream;
struct AVFormatContext;
class QAVCodec;
class QAVStreamPrivate;
class QAVStream
{
public:
    QAVStream();
    QAVStream(int index, AVFormatContext *ctx = nullptr, const QSharedPointer<QAVCodec> &codec = {});
    QAVStream(const QAVStream &other);
    ~QAVStream();
    QAVStream &operator=(const QAVStream &other);
    operator bool() const;

    int index() const;
    AVStream *stream() const;
    double duration() const;
    int64_t framesCount() const;
    double frameRate() const;
    QMap<QString, QString> metadata() const;

    QSharedPointer<QAVCodec> codec() const;

    class Progress
    {
    public:
        Progress(double duration = 0.0, qint64 frames = 0, double fr = 0.0);
        Progress(const Progress &other);
        Progress &operator=(const Progress &other);

        double pts() const;
        double duration() const;
        qint64 framesCount() const;
        qint64 expectedFramesCount() const;
        double frameRate() const;
        double expectedFrameRate() const;
        unsigned fps() const;

        void onFrameSent(double pts);
    private:
        double m_pts = 0.0;
        double m_duration = 0.0;
        qint64 m_framesCount = 0;
        qint64 m_expectedFramesCount = 0;
        double m_expectedFrameRate = 0.0;
        qint64 m_time = 0;
        qint64 m_diffs = 0;
    };

private:
    std::unique_ptr<QAVStreamPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVStream)
};

bool operator==(const QAVStream &lhs, const QAVStream &rhs);

Q_DECLARE_METATYPE(QAVStream)

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug, const QAVStream &);
QDebug operator<<(QDebug, const QAVStream::Progress &);
#endif

QT_END_NAMESPACE

#endif
