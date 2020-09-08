/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qavdemuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavhwdevice_p.h"
#include <QtAVPlayer/private/qtavplayerglobal_p.h>

#if QT_CONFIG(va_x11) && QT_CONFIG(opengl)
#include "qavhwdevice_vaapi_x11_glx_p.h"
#endif

#if QT_CONFIG(va_drm) && QT_CONFIG(egl)
#include "qavhwdevice_vaapi_drm_egl_p.h"
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include "qavhwdevice_videotoolbox_p.h"
#endif

#if defined(Q_OS_WIN)
#include "qavhwdevice_d3d11_p.h"
#endif

#include <QAtomicInt>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

class QAVDemuxerPrivate
{
    Q_DECLARE_PUBLIC(QAVDemuxer)
public:
    QAVDemuxerPrivate(QAVDemuxer *q)
        : q_ptr(q)
    {
    }

    QAVDemuxer *q_ptr = nullptr;
    AVFormatContext *ctx = nullptr;

    QAtomicInt abortRequest = 0;
    bool seekable = false;
    QList<int> audioStreams;
    QList<int> videoStreams;
    QList<int> subtitleStreams;
    int audioStream = -1;
    int videoStream = -1;
    int subtitleStream = -1;
    QScopedPointer<QAVAudioCodec> audioCodec;
    QScopedPointer<QAVVideoCodec> videoCodec;
    QScopedPointer<QAVCodec> subtitleCodec;

    AVPacket audioPacket;
    AVPacket videoPacket;
    AVPacket subtitlePacket;
    bool eof = false;
};

static int decode_interrupt_cb(void *ctx)
{
    auto d = reinterpret_cast<QAVDemuxerPrivate *>(ctx);
    return d ? int(d->abortRequest) : 0;
}

QAVDemuxer::QAVDemuxer(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVDemuxerPrivate(this))
{
#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100))
    static bool loaded = false;
    if (!loaded) {
        av_register_all();
        avcodec_register_all();
        loaded = true;
    }
#endif
}

QAVDemuxer::~QAVDemuxer()
{
    unload();
}

void QAVDemuxer::abort(bool stop)
{
    Q_D(QAVDemuxer);
    d->abortRequest = stop;
}

static bool create_device(QAVVideoCodec *codec)
{
    if (qEnvironmentVariableIsSet("QT_AVPLAYER_NO_HWDEVICE"))
        return true;

    const char *hw_device = nullptr;
    AVDictionary *opts = NULL;
    QScopedPointer<QAVHWDevice> device;
    auto name = QGuiApplication::platformName();
#if QT_CONFIG(va_x11) && QT_CONFIG(opengl)
    if (name == QLatin1String("xcb")) {
        device.reset(new QAVHWDevice_VAAPI_X11_GLX);
        av_dict_set(&opts, "connection_type", "x11", 0);
        hw_device = "vaapi";
    }
#endif
#if QT_CONFIG(va_drm) && QT_CONFIG(egl)
    if (name == QLatin1String("eglfs")) {
        device.reset(new QAVHWDevice_VAAPI_DRM_EGL);
        hw_device = "vaapi";
    }
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (name == QLatin1String("cocoa") || name == QLatin1String("ios")) {
        device.reset(new QAVHWDevice_VideoToolbox);
        hw_device = "videotoolbox";
    }
#endif
#if defined(Q_OS_WIN)
    if (name == QLatin1String("windows")) {
        device.reset(new QAVHWDevice_D3D11);
        hw_device = "d3d11va";
    }
#endif

    if (!hw_device)
        return true;

    auto type = av_hwdevice_find_type_by_name(hw_device);
    if (type == AV_HWDEVICE_TYPE_NONE) {
        qWarning() << "Device type is not supported.";
        qWarning() << "Available device types:";
        while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            qWarning() << av_hwdevice_get_type_name(type);
    }

    if (type == AV_HWDEVICE_TYPE_NONE)
        return false;

    AVPixelFormat hw_format = AV_PIX_FMT_NONE;
    for (int i = 0;; ++i) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec->codec(), i);
        if (!config)
            break;

        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX
            && config->device_type == type)
        {
            hw_format = config->pix_fmt;
            break;
        }
    }

    if (hw_format == AV_PIX_FMT_NONE) {
        qWarning("Decoder %s does not support device type %s.",
                 codec->codec()->name, av_hwdevice_get_type_name(type));
        return false;
    }

    AVBufferRef *hw_device_ctx = nullptr;
    if (device && av_hwdevice_ctx_create(&hw_device_ctx, type, nullptr, opts, 0) >= 0) {
        qDebug() << "Using hardware device context:" << av_hwdevice_get_type_name(type);
        codec->avctx()->hw_device_ctx = hw_device_ctx;
        codec->setDevice(device.take());
        return true;
    }

    return false;
}

int QAVDemuxer::load(const QUrl &url)
{
    Q_D(QAVDemuxer);

    if (!d->ctx)
        d->ctx = avformat_alloc_context();

    d->ctx->flags |= AVFMT_FLAG_GENPTS;
    d->ctx->interrupt_callback.callback = decode_interrupt_cb;
    d->ctx->interrupt_callback.opaque = d;

    QString urlString = url.isLocalFile()
                      ? QDir::toNativeSeparators(url.toLocalFile())
                      : url.toString();

    int ret = avformat_open_input(&d->ctx, urlString.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0)
        return ret;

    ret = avformat_find_stream_info(d->ctx, NULL);
    if (ret < 0)
        return ret;

    for (unsigned int i = 0; i < d->ctx->nb_streams; ++i) {
        enum AVMediaType type = d->ctx->streams[i]->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO)
            d->videoStreams.push_back(i);
        else if (type == AVMEDIA_TYPE_AUDIO)
            d->audioStreams.push_back(i);
        else if (type == AVMEDIA_TYPE_SUBTITLE)
            d->subtitleStreams.push_back(i);
    }

    d->videoStream = av_find_best_stream(d->ctx, AVMEDIA_TYPE_VIDEO,
                                         d->videoStream, -1, nullptr, 0);
    d->audioStream = av_find_best_stream(d->ctx, AVMEDIA_TYPE_AUDIO,
                                         d->audioStream,
                                         d->videoStream,
                                         nullptr, 0);
    d->subtitleStream = av_find_best_stream(d->ctx, AVMEDIA_TYPE_SUBTITLE,
                                            d->subtitleStream,
                                            d->audioStream >= 0 ? d->audioStream : d->videoStream,
                                            nullptr, 0);
    if (d->audioStream >= 0) {
        d->audioCodec.reset(new QAVAudioCodec);
        if (!d->audioCodec->open(d->ctx->streams[d->audioStream]))
            qWarning() << "Could not open audio codec";
    }

    if (d->videoStream >= 0) {
        if (!d->videoCodec)
            d->videoCodec.reset(new QAVVideoCodec);
        if (d->videoCodec->open(d->ctx->streams[d->videoStream])) {
            if (!create_device(d->videoCodec.data()))
                qWarning() << "Could not create hardware device";
        } else {
            qWarning() << "Could not open the video codec";
            d->videoCodec.reset();
        }
    }

    d->seekable = d->ctx->iformat->read_seek || d->ctx->iformat->read_seek2;
    if (d->ctx->pb)
        d->seekable |= bool(d->ctx->pb->seekable);

    return 0;
}

int QAVDemuxer::videoStream() const
{
    return d_func()->videoStream;
}

void QAVDemuxer::setVideoCodec(QAVVideoCodec *codec)
{
    return d_func()->videoCodec.reset(codec);
}

QAVVideoCodec *QAVDemuxer::videoCodec() const
{
    return d_func()->videoCodec.data();
}

int QAVDemuxer::audioStream() const
{
    return d_func()->audioStream;
}

QAVAudioCodec *QAVDemuxer::audioCodec() const
{
    return d_func()->audioCodec.data();
}

int QAVDemuxer::subtitleStream() const
{
    return d_func()->subtitleStream;
}

void QAVDemuxer::unload()
{
    Q_D(QAVDemuxer);
    if (d->ctx)
        avformat_close_input(&d->ctx);
    d->eof = false;
    d->abortRequest = 0;
    d->videoStream = -1;
    d->videoStreams.clear();
    d->audioStream = -1;
    d->audioStreams.clear();
    d->subtitleStream = -1;
    d->subtitleStreams.clear();
    d->audioCodec.reset();
    d->videoCodec.reset();
    d->subtitleCodec.reset();
}

bool QAVDemuxer::eof() const
{
    Q_D(const QAVDemuxer);
    return d->eof;
}

QAVPacket QAVDemuxer::read()
{
    Q_D(QAVDemuxer);

    if (!d->ctx || d->eof)
        return {};

    QAVPacket pkt;
    int ret = av_read_frame(d->ctx, pkt.packet());
    if (ret < 0) {
        if (ret == AVERROR_EOF || avio_feof(d->ctx->pb))
            d->eof = true;

        return {};
    }

    if (pkt.packet()->stream_index == d->videoStream)
        pkt.setCodec(d->videoCodec.data());
    if (pkt.packet()->stream_index == d->audioStream)
        pkt.setCodec(d->audioCodec.data());
    if (pkt.packet()->stream_index == d->subtitleStream)
        pkt.setCodec(d->subtitleCodec.data());

    return pkt;
}

bool QAVDemuxer::seekable() const
{
    return d_func()->seekable;
}

int QAVDemuxer::seek(double sec)
{
    Q_D(QAVDemuxer);
    if (!d->ctx || !d->seekable)
        return -1;

    d->eof = false;
    int flags = 0;
    int64_t target = sec * AV_TIME_BASE;
    int64_t min = target;
    int64_t max = target;
    return avformat_seek_file(d->ctx, -1, min, target, max, flags);
}

double QAVDemuxer::duration() const
{
    Q_D(const QAVDemuxer);
    if (!d->ctx || d->ctx->duration == AV_NOPTS_VALUE)
        return 0;

    return d->ctx->duration * av_q2d({1, AV_TIME_BASE});
}

double QAVDemuxer::frameRate() const
{
    Q_D(const QAVDemuxer);
    if (d->videoStream < 0)
        return 1/24;

    AVRational frame_rate = av_guess_frame_rate(d->ctx, d->ctx->streams[d->videoStream], NULL);
    return frame_rate.num && frame_rate.den ? av_q2d({frame_rate.den, frame_rate.num}) : 0;
}

QT_END_NAMESPACE
