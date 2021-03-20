/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideobuffer_cpu_p.h"

extern "C" {
#include <libavutil/imgutils.h>
}

QT_BEGIN_NAMESPACE

QAVVideoBuffer_CPU::QAVVideoBuffer_CPU()
{
}

QAVVideoBuffer_CPU::QAVVideoBuffer_CPU(const QAVVideoFrame &frame)
    : m_frame(frame)
{
}

QAVVideoFrame::MapData QAVVideoBuffer_CPU::map() const
{
    QAVVideoFrame::MapData mapData;
    auto frame = m_frame.frame();

    mapData.size = av_image_get_buffer_size(AVPixelFormat(frame->format), frame->width, frame->height, 1);

    for (int i = 0; i < 4; ++i) {
        if (!frame->linesize[i])
            break;

        mapData.bytesPerLine[i] = frame->linesize[i];
        mapData.data[i] = static_cast<uchar *>(frame->data[i]);
    }

    return mapData;
}

QT_END_NAMESPACE
