/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiooutput.h"
#include <QAudioOutput>
#include <QDebug>

extern "C" {
#include "libavutil/time.h"
}

QT_BEGIN_NAMESPACE

class QAVAudioOutputPrivate
{
public:
    QScopedPointer<QAudioOutput> audioOutput;
    QAudioFormat format;
    qreal volume = 1.0;
    QIODevice *device = nullptr;
};

QAVAudioOutput::QAVAudioOutput(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVAudioOutputPrivate)
{
}

QAVAudioOutput::~QAVAudioOutput()
{
}

void QAVAudioOutput::setVolume(qreal v)
{
    Q_D(QAVAudioOutput);
    d->volume = v;
    if (d->audioOutput)
        d->audioOutput->setVolume(v);
}

bool QAVAudioOutput::play(const QAudioBuffer &data)
{
    Q_D(QAVAudioOutput);
    if (!d->audioOutput) {
        d->audioOutput.reset(new QAudioOutput(data.format()));
        d->device = d->audioOutput->start();
        if (!d->device)
            return false;
        d->audioOutput->setVolume(d->volume);
    }

    int pos = 0;
    int size = data.byteCount();
    while (size) {
        if (d->audioOutput->bytesFree() < d->audioOutput->periodSize()) {
            const double refreshRate = 0.01;
            av_usleep((int64_t)(refreshRate * 1000000.0));
            continue;
        }

        int chunk = qMin(size, d->audioOutput->periodSize());
        QByteArray decodedChunk = QByteArray::fromRawData(static_cast<const char *>(data.constData()) + pos, chunk);
        int wrote = d->device->write(decodedChunk);
        if (wrote > 0) {
            pos += chunk;
            size -= chunk;
        }
    }

    return true;
}

QT_END_NAMESPACE
