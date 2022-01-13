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
#include <QObject>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE

class QAVStreamFramePrivate;
class Q_AVPLAYER_EXPORT QAVStreamFrame : public QObject
{
public:
    QAVStreamFrame(QObject *parent = nullptr);
    QAVStreamFrame(const QAVStreamFrame &other);
    ~QAVStreamFrame();
    QAVStreamFrame &operator=(const QAVStreamFrame &other);

    QAVStream stream() const;
    void setStream(const QAVStream &stream);
    operator bool() const;

    double pts() const;
    double duration() const;

protected:
    QAVStreamFrame(QAVStreamFramePrivate &d, QObject *parent = nullptr);

    QScopedPointer<QAVStreamFramePrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVStreamFrame)
};

QT_END_NAMESPACE

#endif
