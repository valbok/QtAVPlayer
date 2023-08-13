/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVVIDEOBUFFER_P_H
#define QAVVIDEOBUFFER_P_H

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

#include <QtAVPlayer/qavvideoframe.h>
#include <QVariant>

QT_BEGIN_NAMESPACE

class QRhi;
class QAVVideoBuffer
{
public:
    QAVVideoBuffer() = default;
    explicit QAVVideoBuffer(const QAVVideoFrame &frame) : m_frame(frame) { }
    virtual ~QAVVideoBuffer() = default;
    const QAVVideoFrame &frame() const { return m_frame; }

    virtual QAVVideoFrame::MapData map() = 0;
    virtual QAVVideoFrame::HandleType handleType() const { return QAVVideoFrame::NoHandle; }
    virtual QVariant handle(QRhi */*rhi*/ = nullptr) const { return {}; }
protected:
    QAVVideoFrame m_frame;
};

QT_END_NAMESPACE

#endif
