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

#include "qavvideobuffer_cpu_p.h"

QT_BEGIN_NAMESPACE

class Q_AVPLAYER_EXPORT QAVVideoBuffer_GPU
{
public:
    explicit QAVVideoBuffer_GPU(const QAVVideoFrame &frame);
    virtual ~QAVVideoBuffer_GPU();

    QAVVideoFrame::MapData map() const;

    virtual QVariant handle() const;
    const QAVVideoFrame &frame() const { return m_frame; }

private:
    QAVVideoFrame m_frame;
    QAVVideoBuffer_CPU m_cpu;
};

QT_END_NAMESPACE

#endif
