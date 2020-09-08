/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
