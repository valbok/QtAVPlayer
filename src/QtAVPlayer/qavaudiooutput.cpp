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
    QAudioFormat currentFormat;

    void resetIfNeeded(const QAudioFormat &fmt, int bsize, qreal v)
    {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        auto audioDevice = QAudioDeviceInfo::defaultOutputDevice();
#else
        auto audioDevice = QMediaDevices::defaultAudioOutput();
#endif

        if (!audioOutput
            || !fmt.isValid()
            || (fmt.isValid() && audioOutput->format() != fmt)
            || audioOutput->state() == QAudio::StoppedState
            || defaultAudioDevice != audioDevice)
        {
            if (audioOutput) {
                audioOutput->stop();
                audioOutput->deleteLater();
            }

            audioOutput = new AudioOutput(audioDevice, fmt);
            audioOutput->moveToThread(audioThread.get());
            defaultAudioDevice = audioDevice;
            if (bsize > 0)
                audioOutput->setBufferSize(bsize);
            audioOutput->setVolume(v);
            audioOutput->start(device.get());
        }
    }

    void startThreadIfNeeded()
    {
        if (!audioThread) {
            audioThread.reset(new QThread);
            QObject::connect(audioThread.get(), &QThread::started, q_ptr, &QAVAudioOutput::startThread, Qt::DirectConnection);
            QObject::connect(audioThread.get(), &QThread::finished, q_ptr, [this] {
                qDebug()<<__FUNCTION__<<__LINE__<<"-------------";
                audioOutput->reset();
                audioOutput->stop();
                audioOutput->deleteLater();
                audioOutput = nullptr;
            });
            audioThread->start();
        }
    }

    void stopThread()
    {
        if (audioOutput) {
            /*audioOutput->reset();
            //audioOutput->stop();
            audioOutput->deleteLater();
            audioOutput = nullptr;*/
        }
        if (audioThread) {
            audioThread->quit();
            audioThread->wait();
            audioThread = nullptr;
        }
    }
};

QAVAudioOutput::QAVAudioOutput(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVAudioOutputPrivate(this))
{
    Q_D(QAVAudioOutput);
    d->device.reset(new QAVAudioOutputDevice);
    d->device->open(QIODevice::ReadOnly);
}

QAVAudioOutput::~QAVAudioOutput()
{
    Q_D(QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    d->stopThread();
}

void QAVAudioOutput::startThread()
{
    Q_D(QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    d->resetIfNeeded(d->currentFormat, d->bufferSize, d->volume);
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
    auto fmt = format(frame.format());
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    fmt.setChannelConfig(d->channelConfig);
#endif
    QMutexLocker locker(&d->mutex);
    if (fmt != d->currentFormat) {
        d->currentFormat = fmt;
        d->stopThread();
    }
    d->startThreadIfNeeded();
    return d->device->play(frame);
}

void QAVAudioOutput::stop()
{
    Q_D(QAVAudioOutput);
    QMutexLocker locker(&d->mutex);
    d->stopThread();
}

QT_END_NAMESPACE
