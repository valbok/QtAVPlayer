/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVHWDEVICE_MEDIACODEC_P_H
#define QAVHWDEVICE_MEDIACODEC_P_H

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
#include <memory>

QT_BEGIN_NAMESPACE

class QAVHWDevice_MediaCodecPrivate;
class QAVHWDevice_MediaCodec : public QAVHWDevice
{
public:
    QAVHWDevice_MediaCodec();
    ~QAVHWDevice_MediaCodec();

    void init(AVCodecContext *avctx) override;
    AVPixelFormat format() const override;
    AVHWDeviceType type() const override;
    QAVVideoBuffer *videoBuffer(const QAVVideoFrame &frame) const override;

private:
    std::unique_ptr<QAVHWDevice_MediaCodecPrivate> d_ptr;
    Q_DISABLE_COPY(QAVHWDevice_MediaCodec)
    Q_DECLARE_PRIVATE(QAVHWDevice_MediaCodec)
};

QT_END_NAMESPACE

#endif
