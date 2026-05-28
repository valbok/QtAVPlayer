/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVMUXER_P_P_H
#define QAVMUXER_P_P_H

#include <QtAVPlayer/qavmuxer.h>
#include <QObject>
#include <QMap>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE

class QAVMuxerPrivate : public QObject
{
    Q_DECLARE_PUBLIC(QAVMuxer)
public:
    QAVMuxerPrivate(QAVMuxer *q)
        : q_ptr(q)
    {
    }

    int outputStreamIndex(const QAVStream &stream, QAVMuxer::Locker &locker) const;
    static char *err2str(int errnum);

    QAVMuxer *q_ptr = nullptr;
    // Mapping between input frame and output stream index in ctx->streams.
    // Allows to mux frames or packets from different sources.
    QMap<AVStream *, int> outputStreams;
    QString filename;
    QSharedPointer<QAVFormatContext> ctx;
    bool loaded = false;
    mutable QMutex mutex;
};

QT_END_NAMESPACE

#endif
