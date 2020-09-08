/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVPLANARVIDEOBUFFER_GPU_P_H
#define QAVPLANARVIDEOBUFFER_GPU_P_H

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

#include "qavplanarvideobuffer_cpu_p.h"

QT_BEGIN_NAMESPACE

class Q_AVPLAYER_EXPORT QAVPlanarVideoBuffer_GPU : public QAbstractPlanarVideoBuffer
{
public:
    QAVPlanarVideoBuffer_GPU(const QAVVideoFrame &frame, HandleType type = NoHandle);

    MapMode mapMode() const override;
    int map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]) override;
    void unmap() override;

    const QAVVideoFrame &frame() const { return m_frame; }

private:
    QAVVideoFrame m_frame;
    QAVPlanarVideoBuffer_CPU m_cpu;
};

QT_END_NAMESPACE

#endif
