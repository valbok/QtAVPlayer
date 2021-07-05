/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFAUDIORAME_H
#define QAVFAUDIORAME_H

#include <QtAVPlayer/qavframe.h>
#include <QtAVPlayer/qavaudioformat.h>

QT_BEGIN_NAMESPACE

class QAVAudioCodec;
class QAVAudioFramePrivate;
class Q_AVPLAYER_EXPORT QAVAudioFrame : public QAVFrame
{
public:
    QAVAudioFrame(QObject *parent = nullptr);
    ~QAVAudioFrame();
    QAVAudioFrame(const QAVFrame &other, QObject *parent = nullptr);
    QAVAudioFrame(const QAVAudioFrame &other, QObject *parent = nullptr);
    QAVAudioFrame &operator=(const QAVFrame &other);
    QAVAudioFrame &operator=(const QAVAudioFrame &other);

    QAVAudioFormat format() const;
    QByteArray data() const;

private:
    Q_DECLARE_PRIVATE(QAVAudioFrame)
};

Q_DECLARE_METATYPE(QAVAudioFrame)

QT_END_NAMESPACE

#endif
