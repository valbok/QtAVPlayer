/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVAUDIOCODEC_P_H
#define QAVAUDIOCODEC_P_H

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

#include "qavframecodec_p.h"
#include "qavaudioformat.h"

QT_BEGIN_NAMESPACE

class QAVAudioCodec : public QAVFrameCodec
{
public:
    QAVAudioCodec();
    QAVAudioFormat audioFormat() const;

private:
    Q_DISABLE_COPY(QAVAudioCodec)
};

QT_END_NAMESPACE

#endif
