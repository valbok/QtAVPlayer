/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplanarvideobuffer_cpu_p.h"
#include "qavvideoframe_p.h"
#include <QVideoFrame>
#include <QDebug>

extern "C" {
#include <libavutil/imgutils.h>
}

QT_BEGIN_NAMESPACE

QAVPlanarVideoBuffer_CPU::QAVPlanarVideoBuffer_CPU(HandleType type)
    : QAbstractPlanarVideoBuffer(type)
{
}

QAVPlanarVideoBuffer_CPU::QAVPlanarVideoBuffer_CPU(const QAVVideoFrame &frame, HandleType type)
    : QAbstractPlanarVideoBuffer(type)
    , m_frame(frame)
{
}

QAbstractVideoBuffer::MapMode QAVPlanarVideoBuffer_CPU::mapMode() const
{
    return m_mode;
}

int QAVPlanarVideoBuffer_CPU::map(MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4])
{
    auto frame = m_frame.frame();
    if (m_mode != NotMapped || !frame || mode == NotMapped)
        return 0;

    m_mode = mode;
    if (numBytes)
        *numBytes = av_image_get_buffer_size(AVPixelFormat(frame->format), frame->width, frame->height, 1);

    int i = 0;
    for (; i < 4; ++i) {
        if (!frame->linesize[i])
            break;

        bytesPerLine[i] = frame->linesize[i];
        data[i] = static_cast<uchar *>(frame->data[i]);
    }

    return i;
}

void QAVPlanarVideoBuffer_CPU::unmap()
{
    m_mode = NotMapped;
}

QT_END_NAMESPACE
