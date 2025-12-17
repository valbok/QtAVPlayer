/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVSTREAMFRAME_P_H
#define QAVSTREAMFRAME_P_H

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

#include "qavstream.h"
#include <cmath>

QT_BEGIN_NAMESPACE

class QAVStreamFramePrivate
{
public:
    QAVStreamFramePrivate() = default;
    virtual ~QAVStreamFramePrivate() = default;

    virtual double pts() const { return NAN; }
    virtual double duration() const { return 0.0; }

    QAVStream stream;
};

QT_END_NAMESPACE

#endif
