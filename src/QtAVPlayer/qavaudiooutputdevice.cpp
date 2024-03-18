/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiooutputdevice.h"
#include <QDebug>
#include <QMutex>

QT_BEGIN_NAMESPACE

class QAVAudioOutputDevicePrivate
{
public:
    QList<QAVAudioFrame> frames;
    qint64 offset = 0;
    mutable QMutex mutex;
};

QAVAudioOutputDevice::QAVAudioOutputDevice(QObject *parent)
    : QIODevice(parent)
    , d_ptr(new QAVAudioOutputDevicePrivate)
{
}

QAVAudioOutputDevice::~QAVAudioOutputDevice()
{
}

qint64 QAVAudioOutputDevice::readData(char *data, qint64 len)
{
    Q_D(QAVAudioOutputDevice);
    if (!len)
        return 0;

    QMutexLocker locker(&d->mutex);
    qint64 bytesWritten = 0;
    while (len) {
        if (d->frames.isEmpty()) {
            // Better than waiting for new frames: output silence:
            memset(data, 0, (size_t)len); // If error, output silence (0)
            qWarning() << __FUNCTION__ << "no audio data available, sending silence instead, len:" << len << d->offset;
            bytesWritten += len;
            break;
        }

        auto frame = d->frames.front();
        auto sampleData = frame.data();
        const int toWrite = qMin(sampleData.size() - d->offset, len);
        memcpy(data, sampleData.constData() + d->offset, toWrite);
        bytesWritten += toWrite;
        data += toWrite;
        len -= toWrite;
        d->offset += toWrite;

        if (d->offset >= sampleData.size()) {
            d->offset = 0;
            d->frames.removeFirst();
        }
    }

    return bytesWritten;
}

bool QAVAudioOutputDevice::play(const QAVAudioFrame &frame)
{
    Q_D(QAVAudioOutputDevice);
    if (!frame)
        return false;

    QMutexLocker locker(&d->mutex);
    d->frames.push_back(frame);
    return true;
}

QT_END_NAMESPACE
