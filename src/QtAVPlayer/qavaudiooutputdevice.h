/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVAUDIOOUTPUTDEVICE_H
#define QAVAUDIOOUTPUTDEVICE_H

#include <QtAVPlayer/qavaudioframe.h>
#include <QtAVPlayer/qtavplayerglobal.h>
#include <QIODevice>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVAudioOutputDevicePrivate;
class QAVAudioOutputDevice : public QIODevice
{
    Q_OBJECT
public:
    QAVAudioOutputDevice(QObject *parent = nullptr);
    ~QAVAudioOutputDevice();

    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *, qint64) override { return 0; }
    qint64 size() const override { return 0; }
    qint64 bytesAvailable() const override { return std::numeric_limits<qint64>::max(); }
    bool isSequential() const override { return true; }
    bool atEnd() const override { return false; }

    bool play(const QAVAudioFrame &frame);

protected:
    std::unique_ptr<QAVAudioOutputDevicePrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVAudioOutputDevice)
    Q_DECLARE_PRIVATE(QAVAudioOutputDevice)
};

QT_END_NAMESPACE

#endif
