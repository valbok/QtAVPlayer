/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qaviodevice.h"
#include <QMutex>
#include <QWaitCondition>

extern "C" {
#include <libavformat/avio.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
}

QT_BEGIN_NAMESPACE

struct ReadRequest
{
    ReadRequest() = default;
    ReadRequest(unsigned char *data, int maxSize): data(data), maxSize(maxSize) { }
    int wroteBytes = 0;
    unsigned char *data = nullptr;
    int maxSize = 0;
};

struct SeekRequest
{
    SeekRequest() = default;
    SeekRequest(int64_t offset, int whence): offset(offset), whence(whence) { }
    int64_t offset = 0;
    int whence = 0;
    int64_t pos = -1;
};

class QAVIODevicePrivate
{
    Q_DECLARE_PUBLIC(QAVIODevice)
public:
    explicit QAVIODevicePrivate(QAVIODevice *q, const QSharedPointer<QIODevice> &device)
        : q_ptr(q)
        , device(device)
        , buffer(static_cast<unsigned char*>(av_malloc(buffer_size)))
        , ctx(avio_alloc_context(buffer, static_cast<int>(buffer_size), 0, this, &QAVIODevicePrivate::read, nullptr, !device->isSequential() ? &QAVIODevicePrivate::seek : nullptr))
    {
        if (!device->isSequential())
            ctx->seekable = AVIO_SEEKABLE_NORMAL;
    }

    ~QAVIODevicePrivate()
    {
        av_free(ctx);
    }

    void readData()
    {
        QMutexLocker locker(&mutex);
        // if no request or it is being processed
        if (readRequest.data == nullptr || readRequest.wroteBytes)
            return;

        readRequest.wroteBytes = !device->atEnd() ? device->read((char *)readRequest.data, readRequest.maxSize) : AVERROR_EOF;
        // Unblock the decoder thread when there is available bytes
        if (readRequest.wroteBytes) {
            wakeRead = true;
            waitCond.wakeAll();
        }
    }

    void seekData()
    {
        QMutexLocker locker(&mutex);
        // if no request or it is being processed
        if (wakeSeek)
            return;

        int64_t offset = seekRequest.offset;
        const int whence = seekRequest.whence;
        int64_t pos = 0;
        if (whence == AVSEEK_SIZE) {
            pos = device->size() > 0 ? device->size() : 0;
        } else {
            if (whence == SEEK_END)
                offset = device->size() - offset;
            else if (whence == SEEK_CUR)
                offset = device->pos() + offset;

            pos = device->seek(offset) ? device->pos() : -1;
        }

        seekRequest.pos = pos;
        wakeSeek = true;
        waitCond.wakeAll();
    }

    static int read(void *opaque, unsigned char *data, int maxSize)
    {
        auto d = static_cast<QAVIODevicePrivate *>(opaque);
        QMutexLocker locker(&d->mutex);
        if (d->aborted)
            return ECANCELED;

        d->readRequest = { data, maxSize };
        // When decoder thread is the same as current
        d->wakeRead = false;
        locker.unlock();
        // Reading is done on thread where the object is created
        qtavplayer_invokeMethod(d->q_ptr, [d]() -> void { d->readData(); });
        locker.relock();
        // Blocks until data is available or aborted.
        // Rechecking d->aborted here avoids a lost wakeup if abort()
        // was called between unlocking above and waiting below.
        while (!d->wakeRead && !d->aborted)
            d->waitCond.wait(&d->mutex);

        if (d->aborted) {
            d->readRequest = {};
            return ECANCELED;
        }

        int bytes = d->readRequest.wroteBytes;
        d->readRequest = {};
        return bytes;
    }

    static int64_t seek(void *opaque, int64_t offset, int whence)
    {
        auto d = static_cast<QAVIODevicePrivate *>(opaque);
        QMutexLocker locker(&d->mutex);
        if (d->aborted)
            return ECANCELED;

        d->seekRequest = { offset, whence };
        d->wakeSeek = false;
        locker.unlock();
        qtavplayer_invokeMethod(d->q_ptr, [d]() -> void { d->seekData(); });
        locker.relock();
        // Blocks until seeking is done or aborted.
        // Rechecking d->aborted here avoids a lost wakeup if abort()
        // was called between unlocking above and waiting below.
        while (!d->wakeSeek && !d->aborted)
            d->waitCond.wait(&d->mutex);

        if (d->aborted) {
            d->seekRequest = {};
            return ECANCELED;
        }

        int64_t pos = d->seekRequest.pos;
        d->seekRequest = {};
        return pos;
    }

    size_t buffer_size = 64 * 1024;
    QAVIODevice *q_ptr = nullptr;
    QSharedPointer<QIODevice> device;
    unsigned char *buffer = nullptr;
    AVIOContext *ctx = nullptr;
    mutable QMutex mutex;
    QWaitCondition waitCond;
    bool aborted = false;
    bool wakeRead = false;
    bool wakeSeek = false;
    ReadRequest readRequest;
    SeekRequest seekRequest;
};

QAVIODevice::QAVIODevice(const QSharedPointer<QIODevice> &device, QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVIODevicePrivate(this, device))
{
    connect(device.data(), &QIODevice::readyRead, this, [this] {
        Q_D(QAVIODevice);
        d->readData();
    });
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

void QAVIODevice::setBufferSize(size_t size)
{
    Q_D(QAVIODevice);
    QMutexLocker locker(&d->mutex);
    d->buffer_size = size;
}

size_t QAVIODevice::bufferSize() const
{
    Q_D(const QAVIODevice);
    QMutexLocker locker(&d->mutex);
    return d->buffer_size;
}

QT_END_NAMESPACE
