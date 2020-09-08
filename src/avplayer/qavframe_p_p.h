/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFAUDIORAME_P_P_H
#define QAVFAUDIORAME_P_P_H

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

QT_BEGIN_NAMESPACE

struct AVFrame;
class QAVFrame;
class QAVCodec;
class QAVFramePrivate
{
public:
    virtual ~QAVFramePrivate() = default;

    const QAVCodec *codec = nullptr;
    AVFrame *frame = nullptr;
};

QT_END_NAMESPACE

#endif
