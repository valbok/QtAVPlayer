/***************************************************************
 * Copyright (C) 2020, 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVHWDEVICE_D3D11_P_H
#define QAVHWDEVICE_D3D11_P_H

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

struct AVCodecContext;
class QAVHWDevice_D3D11 : public QAVHWDevice
{
public:
    QAVHWDevice_D3D11() = default;
    ~QAVHWDevice_D3D11() = default;

    void init(AVCodecContext *avctx) override;
    AVPixelFormat format() const override;
    AVHWDeviceType type() const override;
    QAVVideoBuffer *videoBuffer(const QAVVideoFrame &frame) const override;

private:
    Q_DISABLE_COPY(QAVHWDevice_D3D11)
};

class QAVHWDevice_D3D11_GL : public QAVHWDevice
{
public:
    QAVHWDevice_D3D11_GL() = default;
    ~QAVHWDevice_D3D11_GL() = default;

    AVPixelFormat format() const override;
    AVHWDeviceType type() const override;
    QAVVideoBuffer *videoBuffer(const QAVVideoFrame &frame) const override;

private:
    Q_DISABLE_COPY(QAVHWDevice_D3D11_GL)
};

QT_END_NAMESPACE

#endif
