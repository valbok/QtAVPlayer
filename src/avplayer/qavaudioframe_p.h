/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFAUDIORAME_H
#define QAVFAUDIORAME_H

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

#include "qavframe_p.h"
#include <QAudioFormat>

QT_BEGIN_NAMESPACE

class QAVAudioCodec;
class QAVAudioFramePrivate;
class Q_AVPLAYER_EXPORT QAVAudioFrame : public QAVFrame
{
public:
    QAVAudioFrame(QObject *parent = nullptr);
    ~QAVAudioFrame();
    QAVAudioFrame(const QAVFrame &other, QObject *parent = nullptr);
    QAVAudioFrame &operator=(const QAVFrame &other);

    const QAVAudioCodec *codec() const;
    QByteArray data(const QAudioFormat &format) const;

private:
    Q_DECLARE_PRIVATE(QAVAudioFrame)
};

QT_END_NAMESPACE

#endif
