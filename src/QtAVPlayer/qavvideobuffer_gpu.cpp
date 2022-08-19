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
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

QAVVideoFrame::MapData QAVVideoBuffer_GPU::map()
{
    auto mapData = m_cpu.map();
    if (mapData.format == AV_PIX_FMT_NONE) {
        int ret = av_hwframe_transfer_data(m_cpu.frame().frame(), m_frame.frame(), 0);
        if (ret < 0) {
            qWarning() << "Could not av_hwframe_transfer_data:" << ret;
            return {};
        }
        m_frame = QAVVideoFrame();
        mapData = m_cpu.map();
    }

    return mapData;
}

QT_END_NAMESPACE
