/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

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
#include <QSize>

QT_BEGIN_NAMESPACE

class QRhi;
class QRhiTexture;
class Q_AVPLAYER_EXPORT QAVVideoBuffer
{
public:
    QAVVideoBuffer() = default;
    explicit QAVVideoBuffer(const QAVVideoFrame &frame) : m_frame(frame) { }
    virtual ~QAVVideoBuffer() = default;
    const QAVVideoFrame &frame() const { return m_frame; }

    virtual QAVVideoFrame::MapData map() = 0;
    // Returns if the data is mapped to CPU memory
    virtual bool isMapped() const = 0;
    virtual QAVVideoFrame::HandleType handleType() const { return QAVVideoFrame::NoHandle; }
    virtual QVariant handle(QRhi */*rhi*/ = nullptr) const { return {}; }
    // Returns the textures of the planes of the frame, which can be rendered by rhi.
    // The caller takes ownership of the textures and must destroy them in the rhi's thread.
    // Returns an empty list if the frame cannot provide the textures for the given rhi.
    virtual QList<QRhiTexture *> mapRhiTextures(QRhi */*rhi*/) { return {}; }
    // Returns the size of the frame from internal codec
    virtual QSize size() const { return m_frame.size(); }
protected:
    QAVVideoFrame m_frame;
};

QT_END_NAMESPACE

#endif
