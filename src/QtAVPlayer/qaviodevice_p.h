/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFIODEVICE_P_H
#define QAVFIODEVICE_P_H

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QScopedPointer>
#include <QIODevice>

QT_BEGIN_NAMESPACE

struct AVIOContext;
class QAVIODevicePrivate;
class Q_AVPLAYER_EXPORT QAVIODevice : public QObject
{
public:
    QAVIODevice(QIODevice &device, QObject *parent = nullptr);
    ~QAVIODevice();

    AVIOContext *ctx() const;

protected:
    QScopedPointer<QAVIODevicePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QAVIODevice)
};

QT_END_NAMESPACE

#endif
