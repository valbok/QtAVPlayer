/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFRAME_H
#define QAVFRAME_H

#include <QObject>
#include <QSharedPointer>
#include <QScopedPointer>
#include <QtAVPlayer/qtavplayerglobal.h>

QT_BEGIN_NAMESPACE

extern "C" {
#include <libavutil/frame.h>
}

class QAVFramePrivate;
class QAVCodec;
class Q_AVPLAYER_EXPORT QAVFrame : public QObject
{
public:
    QAVFrame(QObject *parent = nullptr);
    QAVFrame(const QSharedPointer<QAVCodec> &codec, QObject *parent = nullptr);
    ~QAVFrame();
    QAVFrame(const QAVFrame &other);
    QSharedPointer<QAVCodec> codec() const;
    QAVFrame &operator=(const QAVFrame &other);
    operator bool() const;
    AVFrame *frame() const;

    void setFrameRate(const AVRational &value);
    void setTimeBase(const AVRational &value);
    double pts() const;
    double duration() const;

protected:
    QScopedPointer<QAVFramePrivate> d_ptr;
    QAVFrame(QAVFramePrivate &d, QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(QAVFrame)
};

QT_END_NAMESPACE

#endif
