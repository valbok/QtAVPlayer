#include "qavhwdevice_dxva2_p.h"
#include "qavvideobuffer_gpu_p.h"

QT_BEGIN_NAMESPACE

QAVHWDevice_DXVA2::QAVHWDevice_DXVA2(QObject *parent)
    : QObject(parent)
{
}

AVPixelFormat QAVHWDevice_DXVA2::format() const
{
    return AV_PIX_FMT_DXVA2_VLD;
}

AVHWDeviceType QAVHWDevice_DXVA2::type() const
{
    return AV_HWDEVICE_TYPE_DXVA2;
}

QAVVideoBuffer *QAVHWDevice_DXVA2::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

QT_END_NAMESPACE
