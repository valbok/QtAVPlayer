/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qaviodevice_p.h"

extern "C" {
#include <libavformat/avio.h>
}

QT_BEGIN_NAMESPACE

class QAVIODevicePrivate
{
public:
    explicit QAVIODevicePrivate(QIODevice &device)
        : device(device)
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
        auto io = static_cast<QAVIODevicePrivate *>(opaque);
        qint64 bytes = io->device.read((char *)data, maxSize);
        return bytes == 0 ? AVERROR_EOF : bytes;
    }

    static int64_t seek(void *opaque, int64_t offset, int whence)
    {
        auto io = static_cast<QAVIODevicePrivate *>(opaque);
        if (whence == AVSEEK_SIZE)
            return io->device.size() > 0 ? io->device.size() : 0;

        if (whence == SEEK_END)
            offset = io->device.size() - offset;
        else if (whence == SEEK_CUR)
            offset = io->device.pos() + offset;

        if (!io->device.seek(offset))
            return -1;

        return io->device.pos();
    }

    const size_t buffer_size = 4 * 1024;
    QIODevice &device;
    unsigned char *buffer = nullptr;
    AVIOContext *ctx = nullptr;
};

QAVIODevice::QAVIODevice(QIODevice &device, QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVIODevicePrivate(device))
{
}

QAVIODevice::~QAVIODevice()
{
}

AVIOContext *QAVIODevice::ctx() const
{
    return d_func()->ctx;
}

QT_END_NAMESPACE
