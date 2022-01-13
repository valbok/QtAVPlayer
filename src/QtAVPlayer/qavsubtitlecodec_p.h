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

#include "qavcodec_p.h"

QT_BEGIN_NAMESPACE

struct AVSubtitle;
struct AVPacket;
class Q_AVPLAYER_EXPORT QAVSubtitleCodec : public QAVCodec
{
public:
    QAVSubtitleCodec(QObject *parent = nullptr);

    bool decode(const AVPacket *pkt, AVSubtitle *frame) const;

private:
    Q_DISABLE_COPY(QAVSubtitleCodec)
};

QT_END_NAMESPACE

#endif
