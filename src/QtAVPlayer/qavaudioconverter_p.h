/*********************************************************
 * Copyright (C) 2024, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFAUDIOCONVERTER_H
#define QAVFAUDIOCONVERTER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtAVPlayer/qavaudioframe.h>

QT_BEGIN_NAMESPACE

class QAVAudioConverterPrivate;
class QAVAudioConverter
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
