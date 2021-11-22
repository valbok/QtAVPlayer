/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qaviodevice_p.h"
#include <QMutex>
#include <QWaitCondition>

extern "C" {
#include <libavformat/avio.h>
}

QT_BEGIN_NAMESPACE

class QAVIODevicePrivate
{
    Q_DECLARE_PUBLIC(QAVIODevice)
public:
    explicit QAVIODevicePrivate(QAVIODevice *q, QIODevice &device)
        : q_ptr(q)
        , device(device)
        , buffer(static_cast<unsigned char*>(av_malloc(buffer_size)))
        , ctx(avio_alloc_context(buffer, buffer_size, 0, this, &QAVIODevicePrivate::read, nullptr, &QAVIODevicePrivate::seek))
    {
        ctx->seekable = AVIO_SEEKABLE_NORMAL;
    }

    ~QAVIODevicePrivate()
    {
        av_free(ctx);
    }

    static int read(void *opaque, unsigned char *data, int maxSize)
    {
        auto d = static_cast<QAVIODevicePrivate *>(opaque);
        QMutexLocker locker(&d->mutex);
        if (d->aborted)
            return ECANCELED;

        int bytes = 0;
        bool wake = false;
        locker.unlock();
        QMetaObject::invokeMethod(d->q_ptr, [&] {
            QMutexLocker locker(&d->mutex);
            bytes = !d->device.atEnd() ? d->device.read((char *)data, maxSize) : AVERROR_EOF;
            d->waitCond.wakeAll();
            wake = true;
        }, nullptr);

        locker.relock();
        if (!wake)
            d->waitCond.wait(&d->mutex);

        return bytes;
    }

    static int64_t seek(void *opaque, int64_t offset, int whence)
    {
        auto d = static_cast<QAVIODevicePrivate *>(opaque);
        QMutexLocker locker(&d->mutex);
        if (d->aborted)
            return ECANCELED;

        int64_t pos = 0;
        bool wake = false;
        locker.unlock();
        QMetaObject::invokeMethod(d->q_ptr, [&] {
            QMutexLocker locker(&d->mutex);
            if (whence == AVSEEK_SIZE) {
                pos = d->device.size() > 0 ? d->device.size() : 0;
            } else {
                if (whence == SEEK_END)
                    offset = d->device.size() - offset;
                else if (whence == SEEK_CUR)
                    offset = d->device.pos() + offset;

                pos = d->device.seek(offset) ? d->device.pos() : -1;
            }
            d->waitCond.wakeAll();
            wake = true;
        }, nullptr);

        locker.relock();
        if (!wake)
            d->waitCond.wait(&d->mutex);

        return pos;
    }

    const size_t buffer_size = 4 * 1024;
    QAVIODevice *q_ptr = nullptr;
    QIODevice &device;
    unsigned char *buffer = nullptr;
    AVIOContext *ctx = nullptr;
    QMutex mutex;
    QWaitCondition waitCond;
    bool aborted = false;
};

QAVIODevice::QAVIODevice(QIODevice &device, QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVIODevicePrivate(this, device))
{
}

QAVIODevice::~QAVIODevice()
{
    abort(true);
}

AVIOContext *QAVIODevice::ctx() const
{
    return d_func()->ctx;
}

void QAVIODevice::abort(bool aborted)
{
    Q_D(QAVIODevice);
    QMutexLocker locker(&d->mutex);
    d->aborted = aborted;
    d->waitCond.wakeAll();
}

QT_END_NAMESPACE
