/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVSUBTITLECODEC_P_H
#define QAVSUBTITLECODEC_P_H

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

#include "qavsubtitleframe.h"
#include "qavcodec_p.h"
#include "qavpacket_p.h"

QT_BEGIN_NAMESPACE

class QAVSubtitleCodecPrivate;
class QAVSubtitleCodec : public QAVCodec
{
public:
    QAVSubtitleCodec();

    int write(const QAVPacket &pkt) override;
    int read(QAVStreamFrame &frame) override;

private:
    Q_DECLARE_PRIVATE(QAVSubtitleCodec)
};

QT_END_NAMESPACE

#endif
