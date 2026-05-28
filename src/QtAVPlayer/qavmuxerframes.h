/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVMUXERFRAMES_H
#define QAVMUXERFRAMES_H

#include <QtAVPlayer/qavmuxer.h>

QT_BEGIN_NAMESPACE

/**
 * Re-encodes the frames using the same encoder
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
    int initStream(const QAVStream &stream, int index, AVStream *out_stream, Locker &) override;
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
