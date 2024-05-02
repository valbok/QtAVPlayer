/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudiooutput.h"
#include "qavaudiooutputdevice.h"
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

class QAVAudioOutputPrivate
{
public:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using AudioOutput = QAudioOutput;
    using AudioDevice = QAudioDeviceInfo;
#else
    using AudioOutput = QAudioSink;
    using AudioDevice = QAudioDevice;
#endif

    QAVAudioOutputPrivate(QAVAudioOutput *q): q_ptr(q) {}

    QAVAudioOutput *q_ptr = nullptr;
    AudioOutput *audioOutput = nullptr;
    qreal volume = 1.0;
    int bufferSize = 0;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    QAudioFormat::ChannelConfig channelConfig = QAudioFormat::ChannelConfigUnknown;
#endif

    std::unique_ptr<QAVAudioOutputDevice> device;
    std::unique_ptr<QThread> audioThread;
    AudioDevice defaultAudioDevice;
    mutable QMutex mutex;

    void resetIfNeeded(const QAudioFormat &fmt, int bsize, qreal v)
    {
        QMutexLocker locker(&mutex);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto audioDevice = QAudioDeviceInfo::defaultOutputDevice();
        auto deviceName = audioDevice.deviceName();
#else
        auto audioDevice = QMediaDevices::defaultAudioOutput();
        auto deviceName = audioDevice.description();
#endif
        if (!audioOutput
            || audioOutput->format() != fmt
            || audioOutput->state() == QAudio::StoppedState
            || defaultAudioDevice != audioDevice)
        {
            if (QThread::currentThread() != audioThread.get()) {
                qWarning() << "QAVAudioOutput initialization must be on the audio thread";
                return;
            }

            if (audioOutput) {
                audioOutput->stop();
                audioOutput->deleteLater();
                audioOutput = nullptr;
            }
            if (audioDevice.isNull() || deviceName.toLower() == QLatin1String("null audio device")) {
                qDebug() << "Audio device is not supported:" << deviceName;
                return;
            }

            audioOutput = new AudioOutput(audioDevice, fmt);
            QObject::connect(audioThread.get(), &QThread::finished, audioOutput, [o=audioOutput] {
                o->stop();
                o->deleteLater();
            });
            defaultAudioDevice = audioDevice;
            if (bsize > 0)
                audioOutput->setBufferSize(bsize);
            audioOutput->setVolume(v);
            // Start sending the audio frames from the queue to render
            device->start();
            audioOutput->start(device.get());
        }
    }
};

QAVAudioOutput::QAVAudioOutput(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVAudioOutputPrivate(this))
{
    Q_D(QAVAudioOutput);
    // The audio is rendered by this thread
    d->audioThread.reset(new QThread);
    moveToThread(d->audioThread.get());
    // QAVAudioOutputDevice::readData() should be called on audioThread
    d->device.reset(new QAVAudioOutputDevice);
    d->device->open(QIODevice::ReadOnly);
    d->audioThread->start();
}

QAVAudioOutput::~QAVAudioOutput()
{
    Q_D(QAVAudioOutput);
    stop();
    d->audioThread->quit();
    d->audioThread->wait();
}

void QAVAudioOutput::setVolume(qreal v)
{
    Q_D(QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    d->volume = v;
    if (d->audioOutput)
        d->audioOutput->setVolume(v);
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
        qWarning() << "QAVAudioOutput: Cannot set buffer size after audioOutput is started";
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
    if (!frame)
        return false;
    auto fmt = format(frame.format());
    if (!fmt.isValid())
        return false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    fmt.setChannelConfig(d->channelConfig);
#endif
    if (QThread::currentThread() == d->audioThread.get()) {
        qCritical() << "QAVAudioOutput::play() must not be called on the audio thread";
    } else {
        quint64 bufferSize = d->bufferSize ? qMin(d->bufferSize, 96000) : 96000;
        if (d->device->bytesInQueue() >= bufferSize) {
            // Reset the output on QAVAudioOutput's thread
            QMetaObject::invokeMethod(this, [fmt, d] {
                d->resetIfNeeded(fmt, d->bufferSize, d->volume);
            });
        }
    }
    // Add frames on current thread
    d->device->play(frame);
    return true;
}

void QAVAudioOutput::stop()
{
    Q_D(QAVAudioOutput);
    d->device->stop();
}

QT_END_NAMESPACE
