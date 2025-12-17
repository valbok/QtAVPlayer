/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFIODEVICE_P_H
#define QAVFIODEVICE_P_H

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QIODevice>
#include <QSharedPointer>
#include <memory>

QT_BEGIN_NAMESPACE

struct AVIOContext;
class QAVIODevicePrivate;
class QAVIODevice : public QObject
{
public:
    QAVIODevice(const QSharedPointer<QIODevice> &device, QObject *parent = nullptr);
    ~QAVIODevice();

    AVIOContext *ctx() const;
    void abort(bool aborted);

    void setBufferSize(size_t size);
    size_t bufferSize() const;

protected:
    std::unique_ptr<QAVIODevicePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QAVIODevice)
};

QT_END_NAMESPACE

#endif
