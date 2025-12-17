/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVVIDEOBUFFER_GPU_P_H
#define QAVVIDEOBUFFER_GPU_P_H

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
#include "qavvideobuffer_cpu_p.h"

QT_BEGIN_NAMESPACE

class QAVVideoBuffer_GPU : public QAVVideoBuffer
{
public:
    QAVVideoBuffer_GPU() = default;
    explicit QAVVideoBuffer_GPU(const QAVVideoFrame &frame) : QAVVideoBuffer(frame) { }
    ~QAVVideoBuffer_GPU() = default;

    QAVVideoFrame::MapData map() override;

protected:
    QAVVideoBuffer_CPU m_cpu;
};

QT_END_NAMESPACE

#endif
