/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVMUXER_H
#define QAVMUXER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qavstream.h"
#include "qavframe.h"
#include "qavsubtitleframe.h"
#include <memory>

struct AVFormatContext;

QT_BEGIN_NAMESPACE

class QAVMuxerPrivate;
/**
 * Allows to mux and write the input streams to output file.
 */
class QAVMuxer
{
public:
    QAVMuxer();
    ~QAVMuxer();

    // Loads the encoder based on parsed streams, format is negotiated from filename
    int load(const QList<QAVStream> &streams, const QString &filename);
    // Stops and unloads the encoder
    void unload();

    // Addes the frame to the queue
    void enqueue(const QAVFrame &frame);
    // Returns size of frames in the queue
    size_t size() const;

    // Directly writes the frame to the encoder
    int write(const QAVFrame &frame);
    int write(const QAVSubtitleFrame &frame);
    // Flushes buffer frames to the encoder
    int flush();

private:
    void close();
    void reset();
    int write(QAVFrame frame, int streamIndex);
    int write(QAVSubtitleFrame frame, int streamIndex);
    void stop();

    Q_DISABLE_COPY(QAVMuxer)
    Q_DECLARE_PRIVATE(QAVMuxer)
    std::unique_ptr<QAVMuxerPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
