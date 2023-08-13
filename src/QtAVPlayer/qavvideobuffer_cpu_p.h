/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVVIDEOBUFFER_CPU_P_H
#define QAVVIDEOBUFFER_CPU_P_H

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

#include "qavvideobuffer_p.h"

QT_BEGIN_NAMESPACE

class QAVVideoBuffer_CPU : public QAVVideoBuffer
{
public:
    QAVVideoBuffer_CPU() = default;
    ~QAVVideoBuffer_CPU() = default;
    explicit QAVVideoBuffer_CPU(const QAVVideoFrame &frame) : QAVVideoBuffer(frame) { }

    QAVVideoFrame::MapData map() override;
};

QT_END_NAMESPACE

#endif
