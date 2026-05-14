/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavaudiooutputdevice_p.h"
#include "qavaudioconverter_p.h"
#include <QDebug>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>

QT_BEGIN_NAMESPACE

class QAVAudioOutputDevicePrivate
{
public:
    QList<QByteArray> frames;
    qint64 offset = 0;
    quint64 bytes = 0;
    std::unique_ptr<QAVAudioConverter> conv;
    mutable QMutex mutex;
    QWaitCondition cond;
    bool quit = false;
    bool flush = false;
};

QAVAudioOutputDevice::QAVAudioOutputDevice(QObject *parent)
    : QIODevice(parent)
    , d_ptr(new QAVAudioOutputDevicePrivate)
{
}

QAVAudioOutputDevice::~QAVAudioOutputDevice()
{
    stop();
}

qint64 QAVAudioOutputDevice::readData(char *data, qint64 len)
{
    Q_D(QAVAudioOutputDevice);
    if (!len)
        return 0;
    QMutexLocker locker(&d->mutex);
    qint64 bytesWritten = 0;
    while (len && !d->quit) {
        if (d->frames.isEmpty()) {
            d->cond.wait(&d->mutex);
            if (d->quit || d->flush || d->frames.isEmpty()) {
                d->flush = true;
                break;
            }
        }
        auto &sampleData = d->frames.front();
        const int toWrite = qMin(sampleData.size() - d->offset, len);
        auto src = sampleData.constData() + d->offset;
        memcpy(data, src, toWrite);
        bytesWritten += toWrite;
        data += toWrite;
        len -= toWrite;
        d->offset += toWrite;
        if (d->offset >= sampleData.size()) {
            d->offset = 0;
            d->bytes -= sampleData.size();
            d->frames.removeFirst();
        }
    }
    if (d->quit || d->flush) {
        // Silence for entire buffer
        memset(data - bytesWritten, 0, static_cast<size_t>(len + bytesWritten));
        bytesWritten = len + bytesWritten;
        d->flush = false;
    }
    return d->quit ? 0 : bytesWritten;
}

void QAVAudioOutputDevice::play(const QAVAudioFrame &frame, const QAVAudioFormat &outputFormat)
{
    Q_D(QAVAudioOutputDevice);
    {
        QMutexLocker locker(&d->mutex);
        if (!d->conv || d->quit)
            return;
        auto data = d->conv->data(frame, outputFormat);
        d->bytes += data.size();
        d->frames.push_back(std::move(data));
    }
    d->cond.wakeAll();
}

void QAVAudioOutputDevice::start()
{
    Q_D(QAVAudioOutputDevice);
    {
        QMutexLocker locker(&d->mutex);
        d->quit = false;
        d->conv = std::make_unique<QAVAudioConverter>();
    }
    d->cond.wakeAll();
}

void QAVAudioOutputDevice::stop()
{
    Q_D(QAVAudioOutputDevice);
    {
        QMutexLocker locker(&d->mutex);
        d->quit = true;
        d->conv.reset();
        d->frames.clear();
        d->offset = 0;
        d->bytes = 0;
    }
    d->cond.wakeAll();
}

void QAVAudioOutputDevice::clear()
{
    Q_D(QAVAudioOutputDevice);
    {
        QMutexLocker locker(&d->mutex);
        d->frames.clear();
        d->offset = 0;
        d->bytes = 0;
    }
    d->cond.wakeAll();
}

void QAVAudioOutputDevice::flush()
{
    Q_D(QAVAudioOutputDevice);
    {
        QMutexLocker locker(&d->mutex);
        d->flush = true;
    }
    d->cond.wakeAll();
}

quint64 QAVAudioOutputDevice::bytesInQueue() const
{
    Q_D(const QAVAudioOutputDevice);
    QMutexLocker locker(&d->mutex);
    return d->bytes;
}

QT_END_NAMESPACE
