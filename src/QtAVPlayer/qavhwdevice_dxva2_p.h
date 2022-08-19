#ifndef QAVHWDEVICE_DXVA2_P_H
#define QAVHWDEVICE_DXVA2_P_H

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

class Q_AVPLAYER_EXPORT QAVHWDevice_DXVA2 : public QObject, public QAVHWDevice
{
public:
    QAVHWDevice_DXVA2(QObject *parent = nullptr);
    ~QAVHWDevice_DXVA2() = default;

    AVPixelFormat format() const override;
    AVHWDeviceType type() const override;
    QAVVideoBuffer *videoBuffer(const QAVVideoFrame &frame) const override;

private:
    Q_DISABLE_COPY(QAVHWDevice_DXVA2)
};

QT_END_NAMESPACE

#endif
