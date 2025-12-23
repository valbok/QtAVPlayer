/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVMUXER_H
#define QAVMUXER_H

#include <qtavplayer/qavstream.h>
#include <qtavplayer/qavframe.h>
#include <qtavplayer/qavsubtitleframe.h>

#include <QMutexLocker>

#include <memory>

QT_BEGIN_NAMESPACE

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using Locker = QMutexLocker;
#else
using Locker = QMutexLocker<QMutex>;
#endif

class QAVPacket;
/**
 * Allows to mux and write the input streams to output file
 */
class QAVMuxerPrivate;
class QTAVPLAYER_EXPORT QAVMuxer
{
public:
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

    virtual void init(Locker &) = 0;
    int initMuxer(Locker &);
    virtual int initMuxer(const QAVStream &stream, AVStream *out_stream, Locker &) = 0;
    virtual int flushFrames(Locker &) = 0;
    virtual void reset(Locker &);
    void close(Locker &);

    Q_DISABLE_COPY(QAVMuxer)
    Q_DECLARE_PRIVATE(QAVMuxer)
    std::unique_ptr<QAVMuxerPrivate> d_ptr;
};

/**
 * Remuxes the same packets from demuxer
 */
class QAVMuxerPackets : public QAVMuxer
{
public:
    QAVMuxerPackets() = default;
    ~QAVMuxerPackets() override;

    // Writes the packet directly to the encoder
    int write(const QAVPacket &packet);

private:
    void init(Locker &) override;
    int initMuxer(const QAVStream &stream, AVStream *out_stream, Locker &) override;
    int flushFrames(Locker &) override;
    // Need to make a copy of packet
    int write(QAVPacket packet, int streamIndex, Locker &);
};

/**
 * Reencodes the frames using the same encoder
 */
class QAVMuxerFramesPrivate;
class QAVMuxerFrames : public QAVMuxer
{
public:
    QAVMuxerFrames();
    ~QAVMuxerFrames() override;
    
    // Adds the frame to the queue
    void enqueue(const QAVFrame &frame);

    // Returns size of frames in the queue
    size_t size() const;

    // Directly writes to the encoder
    int write(const QAVFrame &frame);
    int write(const QAVSubtitleFrame &frame);

private:
    void init(Locker &) override;
    int initMuxer(const QAVStream &stream, AVStream *out_stream, Locker &) override;
    // Need to make a copy of frame
    // streamIndex is needed to flush empty frame
    int write(QAVFrame frame, int streamIndex, Locker &);
    int write(QAVSubtitleFrame frame, int streamIndex, Locker &);
    void stop(Locker &);
    void reset(Locker &) override;
    int flushFrames(Locker &) override;

    Q_DECLARE_PRIVATE(QAVMuxerFrames)
};
QT_END_NAMESPACE

#endif
