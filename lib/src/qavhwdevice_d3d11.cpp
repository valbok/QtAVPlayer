/***************************************************************
 * Copyright (C) 2020, 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavhwdevice_d3d11_p.h"
#include "qavvideobuffer_gpu_p.h"
#include <QDebug>
#include <QOpenGLFunctions>  // GLuint
#include <d3d11.h>
#include <d3d11_3.h>
#include <d3dcompiler.h>
#include <system_error>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#if defined(QT_AVPLAYER_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    #include <private/qrhi_p.h>
    #include <private/qrhid3d11_p.h>
    #if QT_VERSION >= QT_VERSION_CHECK(6, 6, 2)
        #include <private/quniquehandle_p.h>
    #endif
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/d3d11va.h>
#include <libavutil/hwcontext_d3d11va.h>
}

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "D3DCompiler.lib")

typedef HANDLE (*wglDXOpenDeviceNV_)(void *dxDevice);
static wglDXOpenDeviceNV_ s_wglDXOpenDeviceNV = nullptr;

typedef BOOL (*wglDXCloseDeviceNV_)(HANDLE hDevice);
static wglDXCloseDeviceNV_ s_wglDXCloseDeviceNV = nullptr;

typedef HANDLE (*wglDXRegisterObjectNV_)(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
static wglDXRegisterObjectNV_ s_wglDXRegisterObjectNV = nullptr;

typedef BOOL (*wglDXUnregisterObjectNV_)(HANDLE hDevice, HANDLE hObject);
static wglDXUnregisterObjectNV_ s_wglDXUnregisterObjectNV = nullptr;

typedef BOOL (WINAPI* wglDXLockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);
static wglDXLockObjectsNV s_wglDXLockObjectsNV = nullptr;

typedef BOOL (WINAPI* wglDXUnlockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);
static wglDXUnlockObjectsNV s_wglDXUnlockObjectsNV = nullptr;

typedef BOOL (WINAPI* wglDXObjectAccessNV)(HANDLE hObject, GLenum access);
static wglDXObjectAccessNV s_wglDXObjectAccessNV = nullptr;

#define WGL_ACCESS_READ_WRITE_NV          0x00000001

#define ENSURE(f, ...) CHECK(f, return __VA_ARGS__;)
#define CHECK(f, ...) \
    do { \
        HRESULT hr = f; \
        if (FAILED(hr)) { \
            qWarning() << QString::fromLatin1("Error@%1. " #f ": (0x%2)").arg(__LINE__).arg(hr, 0, 16) << qt_error_string(hr); \
            __VA_ARGS__ \
        } \
    } while (0)

QT_BEGIN_NAMESPACE

void QAVHWDevice_D3D11::init(AVCodecContext *avctx)
{
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
}

AVPixelFormat QAVHWDevice_D3D11::format() const
{
    return AV_PIX_FMT_D3D11;
}

AVHWDeviceType QAVHWDevice_D3D11::type() const
{
    return AV_HWDEVICE_TYPE_D3D11VA;
}

static ComPtr<ID3D11Texture2D> shareTexture(ID3D11Device *dev, ID3D11Texture2D *tex)
{
    ComPtr<IDXGIResource> dxgiResource;
    ENSURE(tex->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void **>(dxgiResource.GetAddressOf())), {});
    HANDLE shared = nullptr;
    ENSURE(dxgiResource->GetSharedHandle(&shared), {});
    ComPtr<ID3D11Texture2D> sharedTex;
    ENSURE(dev->OpenSharedResource(shared, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(sharedTex.GetAddressOf())), {});
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
    toDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    toDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    toDesc.Usage = D3D11_USAGE_DEFAULT;
    toDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> copy;
    ENSURE(dev->CreateTexture2D(&toDesc, nullptr, &copy), {});

    ComPtr<ID3D11DeviceContext> ctx;
    dev->GetImmediateContext(ctx.GetAddressOf());
    ctx->CopySubresourceRegion(copy.Get(), 0, 0, 0, 0, from, index, nullptr);
    return copy;
}

#if defined(QT_AVPLAYER_MULTIMEDIA) && QT_VERSION >= QT_VERSION_CHECK(6, 6, 2)

static ComPtr<ID3D11Device1> D3D11Device(QRhi *rhi)
{
    auto native = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
    if (!native) {
        qWarning() << "Failed to get nativeHandles";
        return {};
    }

    ComPtr<ID3D11Device> rhiDevice = static_cast<ID3D11Device *>(native->dev);
    ComPtr<ID3D11Device1> dev;
    ENSURE(rhiDevice.As(&dev), {});
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
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    ENSURE(m_hwctx->device->CreateTexture2D(&texDesc, nullptr, m_srcTex.ReleaseAndGetAddressOf()), false);

    ComPtr<IDXGIResource1> res;
    ENSURE(m_srcTex.As(&res), false);
    ENSURE(res->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &m_sharedHandle), false);
    ENSURE(m_srcTex.As(&m_srcMutex), false);
    m_hwctx->device_context->Flush();
    ENSURE(m_srcMutex->AcquireSync(m_srcKey, INFINITE), false);
    m_hwctx->device_context->CopySubresourceRegion(m_srcTex.Get(), 0, 0, 0, 0, m_texture.Get(), m_textureIndex, nullptr);
    m_srcMutex->ReleaseSync(m_destKey);
    return true;
}

ComPtr<ID3D11Texture2D> QAVVideoFrame_D3D11::copyTexture(const ComPtr<ID3D11Device1> &dev,
                                                         const ComPtr<ID3D11DeviceContext> &ctx)
{
    if (!copyToShared())
        return {};

    ENSURE(dev->OpenSharedResource1(m_sharedHandle.get(), IID_PPV_ARGS(&m_destTex)), {});
    CD3D11_TEXTURE2D_DESC desc{};
    m_destTex->GetDesc(&desc);

    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

    ComPtr<ID3D11Texture2D> outputTex;
    ENSURE(dev->CreateTexture2D(&desc, nullptr, outputTex.ReleaseAndGetAddressOf()), {});
    ENSURE(m_destTex.As(&m_destMutex), {});
    ENSURE(m_destMutex->AcquireSync(m_destKey, INFINITE), {});

    ctx->CopySubresourceRegion(outputTex.Get(), 0, 0, 0, 0, m_destTex.Get(), 0, nullptr);
    m_destMutex->ReleaseSync(m_srcKey);
    return outputTex;
}

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
                const_cast<VideoBuffer_D3D11*>(this)->m_texture = copyTexture(dev, shared.Get(), texture_index);
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
        QList<quint64> textures = {quint64(m_texture.Get()), quint64(m_texture.Get())};
        return QVariant::fromValue(textures);
    }

    ComPtr<ID3D11Texture2D> m_texture;
};

QAVVideoBuffer *QAVHWDevice_D3D11::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_D3D11(frame);
}

#else // #if QT_VERSION >= QT_VERSION_CHECK(6, 6, 2)

QAVVideoBuffer *QAVHWDevice_D3D11::videoBuffer(const QAVVideoFrame &frame) const
{
    return new QAVVideoBuffer_GPU(frame);
}

#endif // #if QT_VERSION >= QT_VERSION_CHECK(6, 6, 2)

static constexpr const char* hlsl = R"(
    Texture2D<float> texY : register(t0);         // Y plane
    Texture2D<float2> texUV : register(t1);       // UV plane

    RWTexture2D<float> outY : register(u0);       // Output Y
    RWTexture2D<float4> outUV : register(u1);     // Output UV

    [numthreads(16, 16, 1)]
    void CSMain(uint3 id : SV_DispatchThreadID)
    {
        uint2 coord = id.xy;

        // Write Y (full size)
        outY[coord] = texY.Load(int3(coord, 0));

        // UV is subsampled vertically (not horizontally!)
        if (coord.y % 2 == 0)
        {
            uint2 uvCoord = coord / uint2(1, 2);
            float2 uv = texUV.Load(int3(uvCoord, 0));
            outUV[uvCoord] = float4(uv.x, 0, 0, uv.y);
        }
    }
)";

class VideoBuffer_D3D11_GL : public QAVVideoBuffer_GPU
{
public:
    static void init()
    {
        if (!s_wglDXOpenDeviceNV) {
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
            s_wglDXOpenDeviceNV = (wglDXOpenDeviceNV_)wglGetProcAddress("wglDXOpenDeviceNV");
            s_wglDXCloseDeviceNV = (wglDXCloseDeviceNV_)wglGetProcAddress("wglDXCloseDeviceNV");
            s_wglDXRegisterObjectNV = (wglDXRegisterObjectNV_)wglGetProcAddress("wglDXRegisterObjectNV");
            s_wglDXUnregisterObjectNV = (wglDXUnregisterObjectNV_)wglGetProcAddress("wglDXUnregisterObjectNV");
            s_wglDXLockObjectsNV = (wglDXLockObjectsNV)wglGetProcAddress("wglDXLockObjectsNV");
            s_wglDXUnlockObjectsNV = (wglDXUnlockObjectsNV)wglGetProcAddress("wglDXUnlockObjectsNV");
            s_wglDXObjectAccessNV = (wglDXObjectAccessNV)wglGetProcAddress("wglDXObjectAccessNV");
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
            createDevice();
            initDebug();
            compileShader();
        }
    }

    static void createDevice()
    {
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef DEBUG
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_1
        };
        CHECK(D3D11CreateDevice(nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            s_d3ddev.ReleaseAndGetAddressOf(),
            nullptr,
            s_d3dctx.ReleaseAndGetAddressOf()));
        s_dxdev = s_wglDXOpenDeviceNV(s_d3ddev.Get());
    }

    static void compileShader()
    {
        CHECK(D3DCompile(
            hlsl,                      // Pointer to the source code
            strlen(hlsl),              // Length of the code
            nullptr,                   // Optional source file name (for error messages)
            nullptr, nullptr,          // Optional macros and include handler
            "CSMain",                  // Entry point
            "cs_5_0",                  // Target profile
            D3DCOMPILE_ENABLE_STRICTNESS,
            0,
            &s_compiledBlob,
            &s_errorBlob
        ));

        CHECK(s_d3ddev->CreateComputeShader(s_compiledBlob->GetBufferPointer(),
            s_compiledBlob->GetBufferSize(),
            nullptr,
            s_computeShader.GetAddressOf()));
        s_d3dctx->CSSetShader(s_computeShader.Get(), nullptr, 0);
    }

    static void initDebug()
    {
#ifdef DEBUG
        ComPtr<ID3D11Debug> debug;
        if (SUCCEEDED(s_d3ddev.As(&debug)))
            CHECK(debug->QueryInterface(IID_PPV_ARGS(&s_infoQueue)));
#endif
    }

    static void printDebug()
    {
#ifdef DEBUG
        Q_ASSERT(s_infoQueue);
        UINT64 messageCount = s_infoQueue->GetNumStoredMessages();
        for (UINT64 i = 0; i < messageCount; i++) {
            SIZE_T messageLength = 0;
            s_infoQueue->GetMessage(i, nullptr, &messageLength);

            D3D11_MESSAGE* message = (D3D11_MESSAGE*)malloc(messageLength);
            if (message) {
                s_infoQueue->GetMessage(i, message, &messageLength);
                printf("D3D11 Debug: %s\n", message->pDescription);
                OutputDebugStringA(message->pDescription);
                free(message);
            }
        }
#endif
    }

    VideoBuffer_D3D11_GL(const QAVVideoFrame &frame)
        : QAVVideoBuffer_GPU(frame)
    {
        init();
    }

    ~VideoBuffer_D3D11_GL()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        for (int i = 0; i < 2 && m_texHD[i]; ++i) {
            s_wglDXUnlockObjectsNV(s_dxdev, 1, &m_texHD[i]);
            s_wglDXUnregisterObjectNV(s_dxdev, m_texHD[i]);
        }
        glDeleteTextures(2, m_texGL);
    }

    QAVVideoFrame::HandleType handleType() const override
    {
        return QAVVideoFrame::GLTextureHandle;
    }

    QList<quint64> textures()
    {
        if (!s_wglDXOpenDeviceNV) {
            qWarning() << "Could not get proc address: s_wglDXOpenDeviceNV";
            return {};
        }

        if (!frame())
            return {};

        if (m_texGL[0])
            return {m_texGL[0], m_texGL[1]};

        // First copy ffmpeg texture to shared one
        const auto av_frame = frame().frame();
        const auto width = av_frame->width;
        const auto height = av_frame->height;

        auto texture = (ID3D11Texture2D *)(uintptr_t)av_frame->data[0];
        auto texture_index = (intptr_t)av_frame->data[1];
        if (!texture) {
            qWarning() << "No texture in the frame" << frame().pts();
            return {};
        }
        auto shared = shareTexture(s_d3ddev.Get(), texture);
        auto outputTex = copyTexture(s_d3ddev.Get(), shared.Get(), texture_index);
        if (!outputTex) {
            qWarning() << "Could not copy d3d11 texture";
            return {};
        }

        // OpenGL does not support NV12 textures natively, so
        // we should return 2 textures: for Y and UV planes.
        // Need to copy planes to specific texture:
        //  1. DXGI_FORMAT_NV12 cannot be used in wglDXRegisterObjectNV
        //  2. Creating few ID3D11Texture2D and copying the data to it
        //     via CopySubresourceRegion(nv12Texture) is not allowed.
        // Solution:
        //  1. Create 2 shared ID3D11Texture2D textures: R8 and BGRA (not R8G8)
        //  2. Create Shader Resource Views (SRV): R8 and R8G8
        //  3. Create Unordered Access Views (UAV) for ID3D11Texture2D textures: plane 0 and plane 1
        //  4. Extract planes in HLSL and copy to ID3D11Texture2D: R8 and BGRA
        //     NOTE: BGRA (and not R8G8) is needed by OpenGL shader used to render in QAVWidget_OpenGL
        //  5. Generate OpenGL textures and bind ID3D11Texture2D R8 and BGRA to OpenGL via wglDXRegisterObjectNV

        // For Y plane (Plane 0)
        D3D11_TEXTURE2D_DESC descY = {};
        descY.Width = width;
        descY.Height = height;
        descY.Format = DXGI_FORMAT_R8_UNORM;
        descY.Usage = D3D11_USAGE_DEFAULT;
        descY.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        descY.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        descY.ArraySize = 1;
        descY.MipLevels = 1;
        descY.SampleDesc.Count = 1;

        ComPtr<ID3D11Texture2D> texY;
        ENSURE(s_d3ddev->CreateTexture2D(&descY, nullptr, &texY), {});

        // For UV plane (Plane 1)
        D3D11_TEXTURE2D_DESC descUV = {};
        descUV.Width = width / 2;
        descUV.Height = height / 2;
        descUV.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        descUV.Usage = D3D11_USAGE_DEFAULT;
        descUV.BindFlags = D3D11_BIND_SHADER_RESOURCE| D3D11_BIND_UNORDERED_ACCESS;
        descUV.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        descUV.ArraySize = 1;
        descUV.MipLevels = 1;
        descUV.SampleDesc.Count = 1;

        ComPtr<ID3D11Texture2D> texUV;
        ENSURE(s_d3ddev->CreateTexture2D(&descUV, nullptr, &texUV), {});

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDescY = {};
        srvDescY.Format = DXGI_FORMAT_R8_UNORM;
        srvDescY.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDescY.Texture2D.MipLevels = 1;
        srvDescY.Texture2D.MostDetailedMip = 0;

        ComPtr<ID3D11ShaderResourceView> srvY;
        ENSURE(s_d3ddev->CreateShaderResourceView(outputTex.Get(), &srvDescY, &srvY), {});

        D3D11_SHADER_RESOURCE_VIEW_DESC1 srvDescUV = {};
        srvDescUV.Format = DXGI_FORMAT_R8G8_UNORM;
        srvDescUV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDescUV.Texture2D.MipLevels = 1;
        srvDescUV.Texture2D.MostDetailedMip = 0;
        srvDescUV.Texture2D.PlaneSlice = 1;

        ComPtr<ID3D11ShaderResourceView1> srvUV;
        ComPtr<ID3D11Device3> dev3;
        ENSURE(s_d3ddev->QueryInterface(IID_PPV_ARGS(&dev3)), {});

        ENSURE(dev3->CreateShaderResourceView1(outputTex.Get(), &srvDescUV, &srvUV), {});

        ComPtr<ID3D11UnorderedAccessView> yUAV, uvUAV;
        ENSURE(s_d3ddev->CreateUnorderedAccessView(texY.Get(), nullptr, &yUAV), {});
        ENSURE(s_d3ddev->CreateUnorderedAccessView(texUV.Get(), nullptr, &uvUAV), {});

        ID3D11ShaderResourceView* srvs[] = { srvY.Get(), srvUV.Get() };
        s_d3dctx->CSSetShaderResources(0, 2, srvs);
        ID3D11UnorderedAccessView* uavs[2] = { yUAV.Get(), uvUAV.Get() };
        s_d3dctx->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

        s_d3dctx->Dispatch((width + 15) / 16, (height + 15) / 16, 1);

        glGenTextures(2, m_texGL);
        m_texHD[0] = s_wglDXRegisterObjectNV(s_dxdev, texY.Get(), m_texGL[0], GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
        s_wglDXLockObjectsNV(s_dxdev, 1, &m_texHD[0]);
        s_wglDXObjectAccessNV(m_texHD[0], WGL_ACCESS_READ_WRITE_NV);

        m_texHD[1] = s_wglDXRegisterObjectNV(s_dxdev, texUV.Get(), m_texGL[1], GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);
        s_wglDXLockObjectsNV(s_dxdev, 1, &m_texHD[1]);
        s_wglDXObjectAccessNV(m_texHD[1], WGL_ACCESS_READ_WRITE_NV);

        // Optionally unbind (to avoid D3D warnings)
        ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
        s_d3dctx->CSSetShaderResources(0, 2, nullSRVs);
        ID3D11UnorderedAccessView* nullUAVs[2] = { nullptr, nullptr };
        s_d3dctx->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);

        return {m_texGL[0], m_texGL[1]};
    }

    QVariant handle(QRhi *) const override
    {
        QList<quint64> v = const_cast<VideoBuffer_D3D11_GL *>(this)->textures();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QList<QVariant> list;
        for (auto &a : v)
            list.append(QVariant(a));
        return list;
#else
        return QVariant::fromValue(v);
#endif
    }

    GLuint m_texGL[2] = {0, 0};
    HANDLE m_texHD[2] = {nullptr, nullptr};

    static ComPtr<ID3D11Device> s_d3ddev;
    static ComPtr<ID3D11DeviceContext> s_d3dctx;
    static ComPtr<ID3DBlob> s_compiledBlob;
    static ComPtr<ID3DBlob> s_errorBlob;
    static ComPtr<ID3D11ComputeShader> s_computeShader;
    static HANDLE s_dxdev;
#ifdef DEBUG
    static ComPtr<ID3D11InfoQueue> s_infoQueue;
#endif
};

ComPtr<ID3D11Device> VideoBuffer_D3D11_GL::s_d3ddev;
ComPtr<ID3D11DeviceContext> VideoBuffer_D3D11_GL::s_d3dctx;
ComPtr<ID3DBlob> VideoBuffer_D3D11_GL::s_compiledBlob;
ComPtr<ID3DBlob> VideoBuffer_D3D11_GL::s_errorBlob;
ComPtr<ID3D11ComputeShader> VideoBuffer_D3D11_GL::s_computeShader;
HANDLE VideoBuffer_D3D11_GL::s_dxdev = 0;
#ifdef DEBUG
ComPtr<ID3D11InfoQueue> VideoBuffer_D3D11_GL::s_infoQueue;
#endif

AVPixelFormat QAVHWDevice_D3D11_GL::format() const
{
    return AV_PIX_FMT_D3D11;
}

AVHWDeviceType QAVHWDevice_D3D11_GL::type() const
{
    return AV_HWDEVICE_TYPE_D3D11VA;
}

QAVVideoBuffer *QAVHWDevice_D3D11_GL::videoBuffer(const QAVVideoFrame &frame) const
{
    return new VideoBuffer_D3D11_GL(frame);
}

QT_END_NAMESPACE
