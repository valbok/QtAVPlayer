/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVAUDIOOUTPUT_H
#define QAVAUDIOOUTPUT_H

#include <QtAVPlayer/qavaudioframe.h>
#include <QtAVPlayer/qtavplayerglobal.h>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE

class QAVAudioOutputPrivate;
class Q_AVPLAYER_EXPORT QAVAudioOutput : public QObject
{
public:
    QAVAudioOutput(QObject *parent = nullptr);
    ~QAVAudioOutput();

    void setVolume(qreal v);
    bool play(const QAVAudioFrame &frame);

protected:
    QScopedPointer<QAVAudioOutputPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVAudioOutput)
    Q_DECLARE_PRIVATE(QAVAudioOutput)
};

QT_END_NAMESPACE

#endif
