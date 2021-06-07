/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVCODEC_P_P_H
#define QAVCODEC_P_P_H

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

struct AVCodec;
struct AVStream;
struct AVCodecContext;
class QAVCodecPrivate
{
public:
    virtual ~QAVCodecPrivate() = default;

    AVCodecContext *avctx = nullptr;
    const AVCodec *codec = nullptr;
    AVStream *stream = nullptr;
};

QT_END_NAMESPACE

#endif
