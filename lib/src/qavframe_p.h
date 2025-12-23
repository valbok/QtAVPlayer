/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFRAME_P_H
#define QAVFRAME_P_H

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

#include "qavstreamframe_p.h"

extern "C" {
#include <libavutil/frame.h>
}

QT_BEGIN_NAMESPACE

struct AVFrame;
class QAVFramePrivate : public QAVStreamFramePrivate
{
public:

    double pts() const override;
    double duration() const override;

    AVFrame *frame = nullptr;
    // Overridden data from filters if any
    AVRational frameRate{};
    AVRational timeBase{};
    // Name of a filter the frame has retrieved from
    QString filterName;
};

QT_END_NAMESPACE

#endif
