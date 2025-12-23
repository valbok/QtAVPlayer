/*********************************************************
 * Copyright (C) 2024, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFAUDIOCONVERTER_H
#define QAVFAUDIOCONVERTER_H

#include <qtavplayer/qavaudioframe.h>

QT_BEGIN_NAMESPACE

class QAVAudioConverterPrivate;
class QTAVPLAYER_EXPORT QAVAudioConverter
{
public:
    QAVAudioConverter();
    ~QAVAudioConverter();

    QByteArray data(const QAVAudioFrame &frame);

private:
    Q_DISABLE_COPY(QAVAudioConverter)
    Q_DECLARE_PRIVATE(QAVAudioConverter)
    std::unique_ptr<QAVAudioConverterPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
