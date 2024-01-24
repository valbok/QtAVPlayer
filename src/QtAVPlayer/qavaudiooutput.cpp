/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiooutput.h"
#include <QDebug>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QFuture>
#include <QWaitCondition>
#include <QCoreApplication>
#include <QThreadPool>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#else
#include <QAudioSink>
#include <QMediaDevices>
#endif

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
        qWarning() << "Could not negotiate output format:" << from.sampleFormat();
        return {};
    }

    return out;
}

class QAVAudioOutputPrivate : public QIODevice
{
public:
    QAVAudioOutputPrivate()
    {
        open(QIODevice::ReadOnly);
        threadPool.setMaxThreadCount(1);
    }

    QFuture<void> audioPlayFuture;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using AudioOutput = QAudioOutput;
    using AudioDevice = QAudioDeviceInfo;
#else
    using AudioOutput = QAudioSink;
    using AudioDevice = QAudioDevice;
#endif
    AudioOutput *audioOutput = nullptr;
    qreal volume = 1.0;
    int bufferSize = 0;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QAudioFormat::ChannelConfig channelConfig = QAudioFormat::ChannelConfigUnknown;
#endif

    QList<QAVAudioFrame> frames;
    qint64 offset = 0;
    bool quit = 0;
    AudioDevice defaultAudioDevice;
    mutable QMutex mutex;
    QWaitCondition cond;
    QThreadPool threadPool;

    qint64 readData(char *data, qint64 len) override
    {
        if (!len)
            return 0;

        QMutexLocker locker(&mutex);
        qint64 bytesWritten = 0;
        while (len && !quit) {
            if (frames.isEmpty()) {
                // Wait for more frames
                if (bytesWritten == 0)
                    cond.wait(&mutex);
                if (frames.isEmpty())
                    break;
            }

            auto frame = frames.front();
            auto sampleData = frame.data();
            const int toWrite = qMin(sampleData.size() - offset, len);
            memcpy(data, sampleData.constData() + offset, toWrite);
            bytesWritten += toWrite;
            data += toWrite;
            len -= toWrite;
            offset += toWrite;

            if (offset >= sampleData.size()) {
                offset = 0;
                frames.removeFirst();
            }
        }

        return bytesWritten;
    }

    qint64 writeData(const char *, qint64) override { return 0; }
    qint64 size() const override { return 0; }
    qint64 bytesAvailable() const override { return std::numeric_limits<qint64>::max(); }
    bool isSequential() const override { return true; }
    bool atEnd() const override { return false; }

    void tryInit(const QAudioFormat &fmt, int bsize, qreal v)
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto audioDevice = QAudioDeviceInfo::defaultOutputDevice();
#else
        auto audioDevice = QMediaDevices::defaultAudioOutput();
#endif

        if (!audioOutput
            || (fmt.isValid() && audioOutput->format() != fmt)
            || audioOutput->state() == QAudio::StoppedState
            || defaultAudioDevice != audioDevice)
        {
            if (audioOutput) {
                audioOutput->stop();
                audioOutput->deleteLater();
            }

            audioOutput = new AudioOutput(audioDevice, fmt);
            defaultAudioDevice = audioDevice;

            QObject::connect(audioOutput, &AudioOutput::stateChanged, audioOutput,
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

            if (bsize > 0)
                audioOutput->setBufferSize(bsize);
            audioOutput->setVolume(v);
            audioOutput->start(this);
        }
    }

    void doPlayAudio()
    {
        while (!quit) {
            QMutexLocker locker(&mutex);
            cond.wait(&mutex);
            auto fmt = !frames.isEmpty() ? format(frames.first().format()) : QAudioFormat();
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
            fmt.setChannelConfig(channelConfig);
#endif
            auto v = volume;
            auto bsize = bufferSize;
            locker.unlock();
            if (fmt.isValid())
                tryInit(fmt, bsize, v);
            if (audioOutput)
                audioOutput->setVolume(v);
            QCoreApplication::processEvents();
        }
        if (audioOutput) {
            audioOutput->stop();
            audioOutput->deleteLater();
        }
        audioOutput = nullptr;
    }

    void startThreadIfNeeded()
    {
        if (!audioPlayFuture.isRunning()) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            audioPlayFuture = QtConcurrent::run(&threadPool, this, &QAVAudioOutputPrivate::doPlayAudio);
#else
            audioPlayFuture = QtConcurrent::run(&threadPool, &QAVAudioOutputPrivate::doPlayAudio, this);
#endif
        }
    }
};

QAVAudioOutput::QAVAudioOutput(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVAudioOutputPrivate)
{
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
    d->cond.wakeAll();
}

qreal QAVAudioOutput::volume() const
{
    Q_D(const QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    return d->volume;
}

void QAVAudioOutput::setBufferSize(int bytes)
{
    Q_D(QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    d->bufferSize = bytes;
    if (d->bufferSize > 0 && d->audioOutput)
        d->audioOutput->setBufferSize(d->bufferSize);
}

int QAVAudioOutput::bufferSize() const
{
    Q_D(const QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    return d->bufferSize;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
void QAVAudioOutput::setChannelConfig(QAudioFormat::ChannelConfig config)
{
    Q_D(QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    d->channelConfig = config;
}

QAudioFormat::ChannelConfig QAVAudioOutput::channelConfig() const
{
    Q_D(const QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    return d->channelConfig;
}

#endif

bool QAVAudioOutput::play(const QAVAudioFrame &frame)
{
    Q_D(QAVAudioOutput);
    if (d->quit || !frame)
        return false;

    QMutexLocker locker(&d->mutex);
    d->startThreadIfNeeded();
    d->frames.push_back(frame);
    d->cond.wakeAll();

    return true;
}

QT_END_NAMESPACE
