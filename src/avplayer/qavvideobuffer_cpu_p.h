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

#include "qavvideoframe.h"

QT_BEGIN_NAMESPACE

class Q_AVPLAYER_EXPORT QAVVideoBuffer_CPU
{
public:
    QAVVideoBuffer_CPU();
    QAVVideoBuffer_CPU(const QAVVideoFrame &frame);

    QAVVideoFrame::MapData map() const;

    const QAVVideoFrame &frame() const { return m_frame; }

private:
    QAVVideoFrame m_frame;
};

QT_END_NAMESPACE

#endif
