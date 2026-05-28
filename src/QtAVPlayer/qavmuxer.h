/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVMUXER_H
#define QAVMUXER_H

#include <QtAVPlayer/qavstream.h>
#include <QtAVPlayer/qavframe.h>
#include <QtAVPlayer/qavsubtitleframe.h>
#include <QMutexLocker>
#include <memory>

QT_BEGIN_NAMESPACE

/**
 * Allows to mux and write the input streams to output file
 */
class QAVMuxerPrivate;
class QAVMuxer
{
public:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using Locker = QMutexLocker;
#else
    using Locker = QMutexLocker<QMutex>;
#endif

    virtual ~QAVMuxer();

    // Loads the encoder based on parsed streams, format is negotiated from filename
    int load(const QList<QAVStream> &streams, const QString &filename);

    // Stops and unloads the encoder
    void unload();

    // Flushes buffer frames to the encoder
    int flush();

protected:
    QAVMuxer();
    QAVMuxer(QAVMuxerPrivate &d);

    int allocFormatContext(const QString &filename, Locker &);
    int initStreams(const QList<QAVStream> &streams, Locker &);
    int writeHeader(Locker &);

    virtual void init(Locker &) = 0;
    virtual int initStream(const QAVStream &stream, int index, AVStream *out_stream, Locker &) = 0;
    virtual int flushFrames(Locker &) = 0;
    virtual void reset(Locker &);
    void close(Locker &);

    Q_DISABLE_COPY(QAVMuxer)
    Q_DECLARE_PRIVATE(QAVMuxer)
    std::unique_ptr<QAVMuxerPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
