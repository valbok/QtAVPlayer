/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFVIDEORAME_H
#define QAVFVIDEORAME_H

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

#include "qavframe_p.h"
#include <QtAVPlayer/private/qtavplayerglobal_p.h>
#include <QVideoFrame>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

class QAVVideoFramePrivate;
class QAVVideoCodec;
class Q_AVPLAYER_EXPORT QAVVideoFrame : public QAVFrame
{
public:
    QAVVideoFrame(QObject *parent = nullptr);
    QAVVideoFrame(const QAVFrame &other, QObject *parent = nullptr);

    QAVVideoFrame &operator=(const QAVFrame &other);

    const QAVVideoCodec *codec() const;
    QSize size() const;

    operator QVideoFrame() const;

    static QVideoFrame::PixelFormat pixelFormat(AVPixelFormat from);
    static AVPixelFormat pixelFormat(QVideoFrame::PixelFormat from);
};

QT_END_NAMESPACE

#endif
