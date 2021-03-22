/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavvideobuffer_gpu_p.h"
#include <QDebug>

extern "C" {
#include <libavutil/hwcontext.h>
}

QT_BEGIN_NAMESPACE

QAVVideoBuffer_GPU::QAVVideoBuffer_GPU(const QAVVideoFrame &frame)
    : m_frame(frame)
{
}

QAVVideoBuffer_GPU::~QAVVideoBuffer_GPU()
{
}

QAVVideoFrame::MapData QAVVideoBuffer_GPU::map() const
{
    int ret = av_hwframe_transfer_data(m_cpu.frame().frame(), m_frame.frame(), 0);
    if (ret < 0)
        return {};

    const_cast<QAVVideoBuffer_GPU*>(this)->m_frame = QAVVideoFrame();
    return m_cpu.map();
}

QVariant QAVVideoBuffer_GPU::handle() const
{
    return {};
}

QT_END_NAMESPACE
