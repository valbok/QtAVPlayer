/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVMUXERSUBTITLEFRAMES_H
#define QAVMUXERSUBTITLEFRAMES_H

#include <QtAVPlayer/qavstream.h>
#include <QtAVPlayer/qavsubtitleframe.h>
#include <QString>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVMuxerSubtitleFramesPrivate;
class QAVMuxerSubtitleFrames
{
public:
    QAVMuxerSubtitleFrames();
    ~QAVMuxerSubtitleFrames();

    int load(const QAVStream &stream);
    void unload();

    int parseText(const QAVSubtitleFrame &frame, QString &out);

private:
    Q_DISABLE_COPY(QAVMuxerSubtitleFrames)
    Q_DECLARE_PRIVATE(QAVMuxerSubtitleFrames)
    std::unique_ptr<QAVMuxerSubtitleFramesPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
