/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiooutput.h"
#include <QAudioOutput>
#include <QAudioFormat>
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

    out.setSampleRate(from.sampleRate());
    out.setChannelCount(from.channelCount());
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setByteOrder(QAudioFormat::LittleEndian);
    out.setCodec(QLatin1String("audio/pcm"));
#endif
    switch (from.sampleFormat()) {
    case QAVAudioFormat::UInt8:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setSampleSize(8);
        out.setSampleType(QAudioFormat::UnSignedInt);
#else
        out.setSampleFormat(QAudioFormat::UInt8);
#endif
        break;
    case QAVAudioFormat::Int16:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setSampleSize(16);
        out.setSampleType(QAudioFormat::SignedInt);
#else
        out.setSampleFormat(QAudioFormat::Int16);
#endif
        break;
    case QAVAudioFormat::Int32:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setSampleSize(32);
        out.setSampleType(QAudioFormat::SignedInt);
#else
        out.setSampleFormat(QAudioFormat::Int32);
#endif
        break;
    case QAVAudioFormat::Float:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setSampleSize(32);
        out.setSampleType(QAudioFormat::Float);
#else
        out.setSampleFormat(QAudioFormat::Float);
#endif
        break;
    default:
        qWarning() << "Could not negotiate output format";
        return {};
    }

    return out;
}

class QAVAudioOutputPrivate
{
public:
    QFuture<void> audioPlayFuture;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using AudioOutput = QAudioOutput;
#else
    using AudioOutput = QAudioSink;
#endif
    QScopedPointer<AudioOutput> audioOutput;
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
            audioOutput.reset(new AudioOutput(fmt));
            QObject::connect(audioOutput.data(), &AudioOutput::stateChanged, audioOutput.data(),
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            if (audioOutput->bytesFree() < audioOutput->periodSize()) {
#else
            if (audioOutput->bytesFree() == 0) {
#endif
                const double refreshRate = 0.01;
                av_usleep((int64_t)(refreshRate * 1000000.0));
                continue;
            }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            int chunk = qMin(size, audioOutput->periodSize());
#else
            int chunk = qMin(size, audioOutput->bytesFree());
#endif
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
