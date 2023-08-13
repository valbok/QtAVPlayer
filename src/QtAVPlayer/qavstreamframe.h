/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVSTREAMFRAME_H
#define QAVSTREAMFRAME_H

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QtAVPlayer/qavstream.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVStreamFramePrivate;
class QAVStreamFrame
{
public:
    QAVStreamFrame();
    QAVStreamFrame(const QAVStreamFrame &other);
    ~QAVStreamFrame();
    QAVStreamFrame &operator=(const QAVStreamFrame &other);

    QAVStream stream() const;
    void setStream(const QAVStream &stream);
    operator bool() const;

    double pts() const;
    double duration() const;

    // Receives a data from the codec from the stream
    int receive();

protected:
    QAVStreamFrame(QAVStreamFramePrivate &d);

    std::unique_ptr<QAVStreamFramePrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVStreamFrame)
};

QT_END_NAMESPACE

#endif
