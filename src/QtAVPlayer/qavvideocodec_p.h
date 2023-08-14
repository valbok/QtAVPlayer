/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVVIDEOCODEC_P_H
#define QAVVIDEOCODEC_P_H

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

#include "qavframecodec_p.h"

QT_BEGIN_NAMESPACE

class QAVVideoCodecPrivate;
class QAVHWDevice;
class QAVVideoCodec : public QAVFrameCodec
{
public:
    QAVVideoCodec();
    ~QAVVideoCodec();

    void setDevice(const QSharedPointer<QAVHWDevice> &d);
    QAVHWDevice *device() const;

private:
    Q_DISABLE_COPY(QAVVideoCodec)
    Q_DECLARE_PRIVATE(QAVVideoCodec)
};

QT_END_NAMESPACE

#endif
