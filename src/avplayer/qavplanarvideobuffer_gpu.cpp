/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplanarvideobuffer_gpu_p.h"
#include "qavvideoframe_p.h"
#include <QDebug>


extern "C" {
#include <libavutil/hwcontext.h>
}

QT_BEGIN_NAMESPACE

QAVPlanarVideoBuffer_GPU::QAVPlanarVideoBuffer_GPU(const QAVVideoFrame &frame, HandleType type)
    : QAbstractPlanarVideoBuffer(type)
    , m_frame(frame)
{
}

QAbstractVideoBuffer::MapMode QAVPlanarVideoBuffer_GPU::mapMode() const
{
    return m_cpu.mapMode();
}

int QAVPlanarVideoBuffer_GPU::map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4])
{
    int ret = av_hwframe_transfer_data(m_cpu.frame().frame(), m_frame.frame(), 0);
    if (ret < 0)
        return 0;

    m_frame = QAVVideoFrame();
    return m_cpu.map(mode, numBytes, bytesPerLine, data);
}

void QAVPlanarVideoBuffer_GPU::unmap()
{
    m_cpu.unmap();
}

QT_END_NAMESPACE
