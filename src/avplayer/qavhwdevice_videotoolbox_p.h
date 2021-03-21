/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVHWDEVICE_VIDEOTOOLBOX_P_H
#define QAVHWDEVICE_VIDEOTOOLBOX_P_H

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

#include "qavhwdevice_p.h"

QT_BEGIN_NAMESPACE

class QAVHWDevice_VideoToolboxPrivate;
class Q_AVPLAYER_EXPORT QAVHWDevice_VideoToolbox : public QObject, public QAVHWDevice
{
public:
    QAVHWDevice_VideoToolbox(QObject *parent = nullptr);
    ~QAVHWDevice_VideoToolbox();

    AVPixelFormat format() const override;
    AVHWDeviceType type() const override;
    QAVVideoFrame::MapData map(const QAVVideoFrame &frame) const override;
    QAVVideoFrame::HandleType handleType() const override;
    QVariant handle(const QAVVideoFrame &frame) const override;

private:
    QScopedPointer<QAVHWDevice_VideoToolboxPrivate> d_ptr;
    Q_DISABLE_COPY(QAVHWDevice_VideoToolbox)
    Q_DECLARE_PRIVATE(QAVHWDevice_VideoToolbox)
};

QT_END_NAMESPACE

#endif
