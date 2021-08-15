/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiooutput.h"
#include <QAudioOutput>
#include <QDebug>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QFuture>
#include <QWaitCondition>

extern "C" {
#include "libavutil/time.h"
}

QT_BEGIN_NAMESPACE

static QAudioFormat format(const QAVAudioFormat &from)
{
    QAudioFormat out;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setSampleRate(from.sampleRate());
    out.setChannelCount(from.channelCount());
    out.setByteOrder(QAudioFormat::LittleEndian);
    out.setCodec(QLatin1String("audio/pcm"));
    switch (from.sampleFormat()) {
    case QAVAudioFormat::UInt8:
        out.setSampleSize(8);
        out.setSampleType(QAudioFormat::UnSignedInt);
        break;
    case QAVAudioFormat::Int16:
        out.setSampleSize(16);
        out.setSampleType(QAudioFormat::SignedInt);
        break;
    case QAVAudioFormat::Int32:
        out.setSampleSize(32);
        out.setSampleType(QAudioFormat::SignedInt);
        break;
    case QAVAudioFormat::Float:
        out.setSampleSize(32);
        out.setSampleType(QAudioFormat::Float);
        break;
    default:
        qWarning() << "Could not negotiate output format";
        return {};
    }
#endif

    return out;
}

class QAVAudioOutputPrivate
{
public:
    QFuture<void> audioPlayFuture;
    QScopedPointer<QAudioOutput> audioOutput;
    QIODevice *device = nullptr;
    qreal volume = 1.0;
    bool quit = 0;
    QMutex mutex;
    QWaitCondition cond;
    QList<QAVAudioFrame> frames;

    void play(const QAVAudioFrame &frame)
    {
        auto fmt = format(frame.format());
        if (!audioOutput || audioOutput->format() != fmt || audioOutput->state() == QAudio::StoppedState) {
            audioOutput.reset(new QAudioOutput(fmt));
            QObject::connect(audioOutput.data(), &QAudioOutput::stateChanged, audioOutput.data(),
                [&](QAudio::State state) {
                    switch (state) {
                        case QAudio::StoppedState:
                            if (audioOutput->error() != QAudio::NoError)
                                qWarning() << "QAudioOutput stopped:" << audioOutput->error();
                            break;
                        default:
                            break;
                    }
                });
            device = audioOutput->start();
        }
        if (!device || audioOutput->state() == QAudio::StoppedState)
            return;

        audioOutput->setVolume(volume);
        auto data = frame.data();
        int pos = 0;
        int size = data.size();
        while (!quit && size) {
            if (audioOutput->bytesFree() < audioOutput->periodSize()) {
                const double refreshRate = 0.01;
                av_usleep((int64_t)(refreshRate * 1000000.0));
                continue;
            }

            int chunk = qMin(size, audioOutput->periodSize());
            QByteArray decodedChunk = QByteArray::fromRawData(static_cast<const char *>(data.constData()) + pos, chunk);
            int wrote = device->write(decodedChunk);
            if (wrote > 0) {
                pos += chunk;
                size -= chunk;
            }
        }
    }

    void doPlayAudio()
    {
        while (!quit) {
            QMutexLocker locker(&mutex);
            cond.wait(&mutex);

            while (!quit && !frames.isEmpty()) {
                QAVAudioFrame frame = frames.takeFirst();
                locker.unlock();
                play(frame);
                locker.relock();
            }
        }
        audioOutput.reset();
    }

};

QAVAudioOutput::QAVAudioOutput(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVAudioOutputPrivate)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d_ptr->audioPlayFuture = QtConcurrent::run(d_ptr.data(), &QAVAudioOutputPrivate::doPlayAudio);
#else
    d_ptr->audioPlayFuture = QtConcurrent::run(&QAVAudioOutputPrivate::doPlayAudio, d_ptr.data());
#endif
}

QAVAudioOutput::~QAVAudioOutput()
{
    Q_D(QAVAudioOutput);
    d->quit = true;
    d->cond.wakeAll();
    d->audioPlayFuture.waitForFinished();
}

void QAVAudioOutput::setVolume(qreal v)
{
    Q_D(QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    d->volume = v;
}

bool QAVAudioOutput::play(const QAVAudioFrame &frame)
{
    Q_D(QAVAudioOutput);
    if (d->quit || !frame)
        return false;

    QMutexLocker locker(&d->mutex);
    d->frames.push_back(frame);
    d->cond.wakeAll();

    return true;
}

QT_END_NAMESPACE
