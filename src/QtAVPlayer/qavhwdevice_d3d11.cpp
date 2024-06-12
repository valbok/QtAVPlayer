/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavhwdevice_d3d11_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <d3d11.h>

#ifdef QT_AVPLAYER_MULTIMEDIA

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
#include <private/qrhi_p.h>
#include <private/qrhid3d11_p.h>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 2)
#include <private/qcomptr_p.h>
#else
#include <private/qwindowsiupointer_p.h>
template <class T>
using ComPtr = QWindowsIUPointer<T>;
#endif // #if QT_VERSION >= QT_VERSION_CHECK(6, 5, 2)

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 2)
#include <private/quniquehandle_p.h>
#endif

#include <system_error>
#endif // #if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)

#endif // QT_AVPLAYER_MULTIMEDIA

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/d3d11va.h>
#include <libavutil/hwcontext_d3d11va.h>
}

QT_BEGIN_NAMESPACE

void QAVHWDevice_D3D11::init(AVCodecContext *avctx)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    int ret = avcodec_get_hw_frames_parameters(avctx,
                                               avctx->hw_device_ctx,
                                               AV_PIX_FMT_D3D11,
                                               &avctx->hw_frames_ctx);

    if (ret < 0) {
        qWarning() << "Failed to allocate HW frames context:" << ret;
        return;
    }

    auto frames_ctx = (AVHWFramesContext *)avctx->hw_frames_ctx->data;
    auto hwctx = (AVD3D11VAFramesContext *)frames_ctx->hwctx;
    hwctx->MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    hwctx->BindFlags = D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE;
    ret = av_hwframe_ctx_init(avctx->hw_frames_ctx);
    if (ret < 0) {
        qWarning() << "Failed to initialize HW frames context:" << ret;
        av_buffer_unref(&avctx->hw_frames_ctx);
    }
#else
    Q_UNUSED(avctx);
#endif
}

AVPixelFormat QAVHWDevice_D3D11::format() const
{
    return AV_PIX_FMT_D3D11;
}

AVHWDeviceType QAVHWDevice_D3D11::type() const
{
    return AV_HWDEVICE_TYPE_D3D11VA;
}

#ifdef QT_AVPLAYER_MULTIMEDIA

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)

template <class T>
static T **address(ComPtr<T> &ptr)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 2)
    return ptr.GetAddressOf();
#else
    return ptr.address();
#endif
}

template <class T>
static T *get(const ComPtr<T> &ptr)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 2)
    return ptr.Get();
#else
    return ptr.get();
#endif
}

static ComPtr<ID3D11Texture2D> shareTexture(ID3D11Device *dev, ID3D11Texture2D *tex)
{
    ComPtr<IDXGIResource> dxgiResource;
    HRESULT hr = tex->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void **>(address(dxgiResource)));
    if (FAILED(hr)) {
        qWarning() << "Failed to obtain resource handle from FFmpeg texture:" << hr << std::system_category().message(hr);
        return {};
    }

    HANDLE shared = nullptr;
    hr = dxgiResource->GetSharedHandle(&shared);
    if (FAILED(hr)) {
        qWarning() << "Failed to obtain shared handle for FFmpeg texture:" << hr << std::system_category().message(hr);
        return {};
    }

    ComPtr<ID3D11Texture2D> sharedTex;
    hr = dev->OpenSharedResource(shared, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(address(sharedTex)));
    if (FAILED(hr))
        qWarning() << "Failed to share FFmpeg texture:" << hr << std::system_category().message(hr);
    return sharedTex;
}

static ComPtr<ID3D11Texture2D> copyTexture(ID3D11Device *dev, ID3D11Texture2D *from, int index)
{
    D3D11_TEXTURE2D_DESC fromDesc = {};
    from->GetDesc(&fromDesc);

    D3D11_TEXTURE2D_DESC toDesc = {};
    toDesc.Width = fromDesc.Width;
    toDesc.Height = fromDesc.Height;
    toDesc.Format = fromDesc.Format;
    toDesc.ArraySize = 1;
    toDesc.MipLevels = 1;
    toDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    toDesc.MiscFlags = 0;
    toDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> copy;
    HRESULT hr = dev->CreateTexture2D(&toDesc, nullptr, address(copy));
    if (FAILED(hr)) {
        qWarning() << "Failed to create texture:" << hr << std::system_category().message(hr);
        return {};
    }

    ComPtr<ID3D11DeviceContext> ctx;
    dev->GetImmediateContext(address(ctx));
    ctx->CopySubresourceRegion(get(copy), 0, 0, 0, 0, from, index, nullptr);
    return copy;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 2)

static ComPtr<ID3D11Device1> D3D11Device(QRhi *rhi)
{
    auto native = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
    if (!native) {
        qWarning() << "Failed to get nativeHandles";
        return {};
    }

    ComPtr<ID3D11Device> rhiDevice = static_cast<ID3D11Device *>(native->dev);
    ComPtr<ID3D11Device1> dev;
    auto hr = rhiDevice.As(&dev);
    if (FAILED(hr)) {
        qWarning() << "Failed to get ID3D11Device1:" << hr << std::system_category().message(hr);
        return nullptr;
    }
    return dev;
}

class QAVVideoFrame_D3D11
{
public:
    QAVVideoFrame_D3D11(const AVFrame *frame)
        : m_texture((ID3D11Texture2D *)(uintptr_t)frame->data[0])
        , m_textureIndex((intptr_t)frame->data[1])
    {
        auto ctx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
        m_hwctx = static_cast<AVD3D11VADeviceContext *>(ctx->device_ctx->hwctx);
    }

    ComPtr<ID3D11Texture2D> copyTexture(const ComPtr<ID3D11Device1> &dev,
                                        const ComPtr<ID3D11DeviceContext> &ctx);
private:
    bool copyToShared();

    const ComPtr<ID3D11Texture2D> m_texture;
    const UINT m_textureIndex = 0;
    AVD3D11VADeviceContext *m_hwctx = nullptr;

    struct SharedTextureHandleTraits
    {
        using Type = HANDLE;
        static Type invalidValue() { return nullptr; }
        static bool close(Type handle) { return CloseHandle(handle) != 0; }
    };
    QUniqueHandle<SharedTextureHandleTraits> m_sharedHandle;

    const UINT m_srcKey = 0;
    ComPtr<ID3D11Texture2D> m_srcTex;
    ComPtr<IDXGIKeyedMutex> m_srcMutex;

    const UINT m_destKey = 1;
    ComPtr<ID3D11Texture2D> m_destTex;
    ComPtr<IDXGIKeyedMutex> m_destMutex;
};

bool QAVVideoFrame_D3D11::copyToShared()
{
    m_hwctx->lock(m_hwctx->lock_ctx);
    QScopeGuard autoUnlock([&] { m_hwctx->unlock(m_hwctx->lock_ctx); });

    CD3D11_TEXTURE2D_DESC desc{};
    m_texture->GetDesc(&desc);

    CD3D11_TEXTURE2D_DESC texDesc{ desc.Format, desc.Width, desc.Height };
    texDesc.MipLevels = 1;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

    auto hr = m_hwctx->device->CreateTexture2D(&texDesc, nullptr, m_srcTex.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to CreateTexture2D:" << hr << std::system_category().message(hr);
        return false;
    }

    ComPtr<IDXGIResource1> res;
    hr = m_srcTex.As(&res);
    if (FAILED(hr)) {
        qWarning() << "Failed to get m_srcTex:" << hr << std::system_category().message(hr);
        return false;
    }

    hr = res->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &m_sharedHandle);
    if (FAILED(hr) || !m_sharedHandle) {
        qWarning() << "Failed to CreateSharedHandle:" << hr << std::system_category().message(hr);
        return false;
    }

    hr = m_srcTex.As(&m_srcMutex);
    if (FAILED(hr) || !m_srcMutex) {
        qWarning() << "Failed to get m_srcMutex:" << hr << std::system_category().message(hr);
        return false;
    }

    m_hwctx->device_context->Flush();

    hr = m_srcMutex->AcquireSync(m_srcKey, INFINITE);
    if (FAILED(hr)) {
        qWarning() << "Failed to AcquireSync:" << hr << std::system_category().message(hr);
        return false;
    }

    m_hwctx->device_context->CopySubresourceRegion(m_srcTex.Get(), 0, 0, 0, 0, m_texture.Get(), m_textureIndex, nullptr);
    m_srcMutex->ReleaseSync(m_destKey);
    return true;
}

ComPtr<ID3D11Texture2D> QAVVideoFrame_D3D11::copyTexture(const ComPtr<ID3D11Device1> &dev,
                                                         const ComPtr<ID3D11DeviceContext> &ctx)
{
    if (!copyToShared())
        return {};

    auto hr = dev->OpenSharedResource1(m_sharedHandle.get(), IID_PPV_ARGS(&m_destTex));
    if (FAILED(hr)) {
        qWarning() << "Failed to OpenSharedResource1:" << hr << std::system_category().message(hr);
        return {};
    }

    CD3D11_TEXTURE2D_DESC desc{};
    m_destTex->GetDesc(&desc);

    desc.MiscFlags = 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    ComPtr<ID3D11Texture2D> outputTex;
    hr = dev->CreateTexture2D(&desc, nullptr, outputTex.ReleaseAndGetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to CreateTexture2D:" << hr << std::system_category().message(hr);
        return {};
    }

    hr = m_destTex.As(&m_destMutex);
    if (FAILED(hr)) {
        qWarning() << "Failed to get m_destMutex:" << hr << std::system_category().message(hr);
        return {};
    }

    hr = m_destMutex->AcquireSync(m_destKey, INFINITE);
    if (FAILED(hr)) {
        qWarning() << "Failed to get AcquireSync:" << hr << std::system_category().message(hr);
        return {};
    }

    ctx->CopySubresourceRegion(outputTex.Get(), 0, 0, 0, 0, m_destTex.Get(), 0, nullptr);
    m_destMutex->ReleaseSync(m_srcKey);
    return outputTex;
}

#endif // #if QT_VERSION >= QT_VERSION_CHECK(6, 6, 2)

class VideoBuffer_D3D11: public QAVVideoBuffer_GPU
{
public:
    VideoBuffer_D3D11(const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
    {
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::D3D11Texture2DHandle;
    }

    QVariant handle(QRhi *rhi) const override
    {
        if (!rhi || rhi->backend() != QRhi::D3D11)
            return {};

        if (!m_texture) {
            if (!frame())
                return {};

            if (frame().format() != AV_PIX_FMT_D3D11) {
                qWarning() << "Only AV_PIX_FMT_D3D11 is supported, but got" << frame().formatName();
                return {};
            }
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 2)
            auto av_frame = frame().frame();
            auto texture = (ID3D11Texture2D *)(uintptr_t)av_frame->data[0];
            auto texture_index = (intptr_t)av_frame->data[1];
            if (!texture) {
                qWarning() << "No texture in the frame" << frame().pts();
                return {};
            }
            auto nh = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
            if (!nh) {
                qWarning() << "No QRhiD3D11NativeHandles";
                return {};
            }

            auto dev = reinterpret_cast<ID3D11Device *>(nh->dev);
            if (!dev) {
                qWarning() << "No ID3D11Device device";
                return {};
            }
            auto shared = shareTexture(dev, texture);
            if (shared)
                const_cast<VideoBuffer_D3D11*>(this)->m_texture = copyTexture(dev, get(shared), texture_index);
#else
            auto devRHI = D3D11Device(rhi);
            if (!devRHI) {
                qWarning() << "No device for RHI";
                return {};
            }
            ComPtr<ID3D11DeviceContext> ctxRHI;
            devRHI->GetImmediateContext(ctxRHI.GetAddressOf());

            QAVVideoFrame_D3D11 videoFrame(frame().frame());
            auto outputTex = videoFrame.copyTexture(devRHI, ctxRHI);
            if (!outputTex) {
                qWarning() << "Could not copy d3d11 texture";
                return {};
            }

            // Keep alive texture while the frame is alive
            const_cast<VideoBuffer_D3D11*>(this)->m_texture = outputTex;
#endif // #if QT_VERSION < QT_VERSION_CHECK(6, 6, 2)
        }

        // Return 2 textures since we explicitly use NV12 pixel format
        QList<quint64> textures = {quint64(get(m_texture)), quint64(get(m_texture))};
        return QVariant::fromValue(textures);
    }

    ComPtr<ID3D11Texture2D> m_texture;
};

QAVVideoBuffer *QAVHWDevice_D3D11::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_D3D11(frame);
}

#else // QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)

QAVVideoBuffer *QAVHWDevice_D3D11::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

#endif

#else // QT_AVPLAYER_MULTIMEDIA

QAVVideoBuffer *QAVHWDevice_D3D11::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

#endif // QT_AVPLAYER_MULTIMEDIA

QT_END_NAMESPACE
