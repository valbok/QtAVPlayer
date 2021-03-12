/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVHWDEVICE_P_H
#define QAVHWDEVICE_P_H

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

#include <QtAVPlayer/private/qtavplayerglobal_p.h>
#include <QVideoFrame>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/hwcontext.h>
}

QT_BEGIN_NAMESPACE

class QAVVideoFrame;
class QAVCodec;
class QAbstractVideoSurface;
class Q_AVPLAYER_EXPORT QAVHWDevice
{
public:
    QAVHWDevice() = default;
    virtual ~QAVHWDevice() = default;

    virtual AVPixelFormat format() const = 0;
    virtual AVHWDeviceType type() const = 0;
    virtual bool supportsVideoSurface(QAbstractVideoSurface *surface) const = 0;
    virtual QVideoFrame decode(const QAVVideoFrame &frame) const = 0;

private:
    Q_DISABLE_COPY(QAVHWDevice)
};

QT_END_NAMESPACE

#endif
