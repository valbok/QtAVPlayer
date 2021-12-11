/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavdemuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavhwdevice_p.h"
#include "qtQtAVPlayer-config_p.h"
#include <QtAVPlayer/qtavplayerglobal.h>
#include "qaviodevice_p.h"

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

#if defined(Q_OS_ANDROID)
#include "qavhwdevice_mediacodec_p.h"
#include <QtCore/private/qjnihelpers_p.h>
extern "C" {
#include "libavcodec/jni.h"
}
#endif

#include <QAtomicInt>
#include <QGuiApplication>
#include <QDir>
#include <QSharedPointer>
#include <QMutexLocker>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
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

    bool abortRequest = false;
    mutable QMutex mutex;

    bool seekable = false;
    QList<int> audioStreams;
    QList<int> videoStreams;
    QList<int> subtitleStreams;
    int audioStream = -1;
    int videoStream = -1;
    int subtitleStream = -1;
    QList<QSharedPointer<QAVAudioCodec>> audioCodecs;
    QList<QSharedPointer<QAVVideoCodec>> videoCodecs;

    AVPacket audioPacket;
    AVPacket videoPacket;
    AVPacket subtitlePacket;
    bool eof = false;
};

static int decode_interrupt_cb(void *ctx)
{
    auto d = reinterpret_cast<QAVDemuxerPrivate *>(ctx);
    QMutexLocker locker(&d->mutex);
    return d ? int(d->abortRequest) : 0;
}

static int streamIndex(const QList<int> &list, int stream)
{
    for (int i = 0; i < list.size(); ++i) {
        if (list[i] == stream)
            return i;
    }

    return -1;
}

QAVDemuxer::QAVDemuxer(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVDemuxerPrivate(this))
{
    static bool loaded = false;
    if (!loaded) {
#if (LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58,9,100))
        av_register_all();
        avcodec_register_all();
#endif
        avdevice_register_all();
        loaded = true;
    }
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

static void setup_video_codec(AVStream *stream, QAVVideoCodec &codec)
{
    QScopedPointer<QAVHWDevice> device;
    AVDictionary *opts = NULL;
    Q_UNUSED(opts);
    auto name = QGuiApplication::platformName();

#if QT_CONFIG(va_x11) && QT_CONFIG(opengl)
    if (name == QLatin1String("xcb")) {
        device.reset(new QAVHWDevice_VAAPI_X11_GLX);
        av_dict_set(&opts, "connection_type", "x11", 0);
    }
#endif
#if QT_CONFIG(va_drm) && QT_CONFIG(egl)
    if (name == QLatin1String("eglfs"))
        device.reset(new QAVHWDevice_VAAPI_DRM_EGL);
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (name == QLatin1String("cocoa") || name == QLatin1String("ios"))
        device.reset(new QAVHWDevice_VideoToolbox);
#endif
#if defined(Q_OS_WIN)
    if (name == QLatin1String("windows"))
        device.reset(new QAVHWDevice_D3D11);
#endif
#if defined(Q_OS_ANDROID)
    if (name == QLatin1String("android")) {
        device.reset(new QAVHWDevice_MediaCodec);
        codec.setCodec(avcodec_find_decoder_by_name("h264_mediacodec"));
        auto vm = QtAndroidPrivate::javaVM();
        av_jni_set_java_vm(vm, NULL);
    }
#endif

    if (!codec.open(stream)) {
        qWarning() << "Could not open video codec for stream:" << stream;
        return;
    }

    if (qEnvironmentVariableIsSet("QT_AVPLAYER_NO_HWDEVICE"))
        return;

    QList<AVHWDeviceType> supported;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 0, 0)
    for (int i = 0;; ++i) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec.codec(), i);
        if (!config)
            break;

        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
            supported.append(config->device_type);
    }

    if (!supported.isEmpty()) {
        qDebug() << codec.codec()->name << ": supported hardware device contexts:";
        for (auto a: supported)
            qDebug() << "   " << av_hwdevice_get_type_name(a);
    } else {
        qWarning() << "None of the hardware accelerations are supported";
        return;
    }
#endif

    if (!device) {
        if (!supported.isEmpty())
            qWarning() << "None of the hardware accelerations was implemented";
        return;
    }

    AVBufferRef *hw_device_ctx = nullptr;
    if (av_hwdevice_ctx_create(&hw_device_ctx, device->type(), nullptr, opts, 0) >= 0) {
        qDebug() << "Found hardware device context:" << av_hwdevice_get_type_name(device->type());
        codec.avctx()->hw_device_ctx = hw_device_ctx;
        codec.avctx()->pix_fmt = device->format();
        codec.setDevice(device.take());
    }
}

static void log_callback(void *ptr, int level, const char *fmt, va_list vl)
{
    if (level > av_log_get_level())
        return;

    va_list vl2;
    char line[1024];
    static int print_prefix = 1;

    va_copy(vl2, vl);
    av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
    va_end(vl2);

    qDebug() << "FFmpeg:" << line;
}

QStringList QAVDemuxer::supportedFormats()
{
    static QStringList values;
    if (values.isEmpty()) {
        const AVInputFormat *fmt = nullptr;
        void *it = nullptr;
        while ((fmt = av_demuxer_iterate(&it))) {
            if (fmt->name)
                values << QString::fromLatin1(fmt->name).split(QLatin1Char(','),
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
                    QString::SkipEmptyParts
#else
                    Qt::SkipEmptyParts
#endif
                );
        }
    }

    return values;
}

QStringList QAVDemuxer::supportedProtocols()
{
    static QStringList values;
    if (values.isEmpty()) {
        void *opq = 0;
        const char *value = nullptr;
        while ((value = avio_enum_protocols(&opq, 0)))
            values << QString::fromUtf8(value);
    }

    return values;
}

struct ParsedURL
{
    QString input;
    QString format;
};

static ParsedURL parse_url(const QString &url)
{
    ParsedURL parsed;
    parsed.input = url.trimmed();
    if (parsed.input[0] != QLatin1Char('-'))
        return parsed;

    QString fn = QLatin1Char(' ') + parsed.input;
    auto parts = fn.split(QLatin1String(" -"));
    QString input;
    QString format;
    for (auto &item : parts) {
        if (item[0] == QLatin1Char('i'))
            input = item.mid(1).trimmed();
        else if (item[0] == QLatin1Char('f'))
            format = item.mid(1).trimmed();
    }

    parsed.input = input;
    parsed.format = format;

    return parsed;
}

int QAVDemuxer::load(const QString &url, QAVIODevice *dev)
{
    Q_D(QAVDemuxer);

    if (!d->ctx)
        d->ctx = avformat_alloc_context();

    d->ctx->flags |= AVFMT_FLAG_GENPTS;
    d->ctx->interrupt_callback.callback = decode_interrupt_cb;
    d->ctx->interrupt_callback.opaque = d;
    if (dev) {
        d->ctx->pb = dev->ctx();
        d->ctx->flags |= AVFMT_FLAG_CUSTOM_IO;
    }

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
    const
#endif
    AVInputFormat *inputFormat = nullptr;
    ParsedURL parsed = parse_url(url);
    if (!parsed.format.isEmpty()) {
        qDebug() << "Loading: -f" << parsed.format << "-i" << parsed.input;
        inputFormat = av_find_input_format(parsed.format.toUtf8().constData());
        if (inputFormat == nullptr) {
            qWarning() << "Could not find input format:" << parsed.format;
            return AVERROR(EINVAL);
        }
    }

    int ret = avformat_open_input(&d->ctx, parsed.input.toUtf8().constData(), inputFormat, nullptr);
    if (ret < 0)
        return ret;

    ret = avformat_find_stream_info(d->ctx, NULL);
    if (ret < 0)
        return ret;

    for (unsigned i = 0; i < d->ctx->nb_streams; ++i) {
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
    av_log_set_callback(log_callback);

    for (int i = 0; i < d->audioStreams.size(); ++i) {
        const int stream = d->audioStreams[i];
        auto codec = new QAVAudioCodec;
        if (!codec->open(d->ctx->streams[stream]))
            qWarning() << "Could not open audio codec for stream:" << stream;
        d->audioCodecs.push_back(QSharedPointer<QAVAudioCodec>(codec));
    }

    for (int i = 0; i < d->videoStreams.size(); ++i) {
        const int stream = d->videoStreams[i];
        auto codec = new QAVVideoCodec;
        setup_video_codec(d->ctx->streams[stream], *codec);
        d->videoCodecs.push_back(QSharedPointer<QAVVideoCodec>(codec));
    }

    d->seekable = d->ctx->iformat->read_seek || d->ctx->iformat->read_seek2;
    if (d->ctx->pb)
        d->seekable |= bool(d->ctx->pb->seekable);

    return 0;
}

QList<int> QAVDemuxer::videoStreams() const
{
    return d_func()->videoStreams;
}

int QAVDemuxer::currentVideoStreamIndex() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->videoStream;
}

QList<int> QAVDemuxer::audioStreams() const
{
    return d_func()->audioStreams;
}

int QAVDemuxer::currentAudioStreamIndex() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->audioStream;
}

QList<int> QAVDemuxer::subtitleStreams() const
{
    return d_func()->subtitleStreams;
}

int QAVDemuxer::currentSubtitleStreamIndex() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->subtitleStream;
}

int QAVDemuxer::videoStreamIndex() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return streamIndex(d->videoStreams, d->videoStream);
}

void QAVDemuxer::setVideoStreamIndex(int stream)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (stream < 0 || stream >= d->videoStreams.size())
        return;

    d->videoStream = d->videoStreams[stream];
}

int QAVDemuxer::audioStreamIndex() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return streamIndex(d->audioStreams, d->audioStream);
}

void QAVDemuxer::setAudioStreamIndex(int stream)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (stream < 0 || stream >= d->audioStreams.size())
        return;

    d->audioStream = d->audioStreams[stream];
}

int QAVDemuxer::subtitleStreamIndex() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return streamIndex(d->subtitleStreams, d->subtitleStream);
}

void QAVDemuxer::setSubtitleStreamIndex(int stream)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (stream < 0 || stream >= d->subtitleStreams.size())
        return;

    d->subtitleStream = d->subtitleStreams[stream];
}

void QAVDemuxer::unload()
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
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
    d->audioCodecs.clear();
    d->videoCodecs.clear();
}

bool QAVDemuxer::eof() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->eof;
}

QAVPacket QAVDemuxer::read()
{
    Q_D(QAVDemuxer);
    const int videoStreamIdx = videoStreamIndex();
    const int audioStreamIdx = audioStreamIndex();
    QMutexLocker locker(&d->mutex);
    if (!d->ctx || d->eof)
        return {};

    QAVPacket pkt;
    locker.unlock();
    int ret = av_read_frame(d->ctx, pkt.packet());
    if (ret < 0) {
        if (ret == AVERROR_EOF || avio_feof(d->ctx->pb)) {
            locker.relock();
            d->eof = true;
        }

        return {};
    }
    locker.relock();

    if (pkt.packet()->stream_index == d->videoStream)
        pkt.setCodec(d->videoCodecs[videoStreamIdx]);
    else if (pkt.packet()->stream_index == d->audioStream)
        pkt.setCodec(d->audioCodecs[audioStreamIdx]);

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
        return AVERROR(EINVAL);

    d->eof = false;
    int flags = AVSEEK_FLAG_BACKWARD;
    int64_t target = sec * AV_TIME_BASE;
    int64_t min = INT_MIN;
    int64_t max = target;
    return avformat_seek_file(d->ctx, -1, min, target, max, flags);
}

double QAVDemuxer::duration() const
{
    Q_D(const QAVDemuxer);
    if (!d->ctx || d->ctx->duration == AV_NOPTS_VALUE)
        return 0.0;

    return d->ctx->duration * av_q2d({1, AV_TIME_BASE});
}

double QAVDemuxer::videoFrameRate() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (d->videoStream < 0)
        return 1 / 24.0;

    AVRational fr = av_guess_frame_rate(d->ctx, d->ctx->streams[d->videoStream], NULL);
    return fr.num && fr.den ? av_q2d({fr.den, fr.num}) : 0.0;
}

AVRational QAVDemuxer::frameRate() const
{
    Q_D(const QAVDemuxer);
    return av_guess_frame_rate(d->ctx, d->ctx->streams[d->videoStream], NULL);
}

QSharedPointer<QAVVideoCodec> QAVDemuxer::videoCodec() const
{
    Q_D(const QAVDemuxer);
    const int videoStreamIdx = videoStreamIndex();
    QMutexLocker locker(&d->mutex);
    return videoStreamIdx >= 0 ? d->videoCodecs[videoStreamIdx] : QSharedPointer<QAVVideoCodec>();
}

QSharedPointer<QAVAudioCodec> QAVDemuxer::audioCodec() const
{
    Q_D(const QAVDemuxer);
    const int audioStreamIdx = audioStreamIndex();
    QMutexLocker locker(&d->mutex);
    return audioStreamIdx >= 0 ? d->audioCodecs[audioStreamIdx] : QSharedPointer<QAVAudioCodec>();
}

AVStream *QAVDemuxer::videoStream() const
{
    Q_D(const QAVDemuxer);
    return d->videoStream >= 0 && d->videoStream < static_cast<int>(d->ctx->nb_streams) ? d->ctx->streams[d->videoStream] : nullptr;
}

AVStream *QAVDemuxer::audioStream() const
{
    Q_D(const QAVDemuxer);
    return d->audioStream >= 0 && d->audioStream < static_cast<int>(d->ctx->nb_streams) ? d->ctx->streams[d->audioStream] : nullptr;
}

QT_END_NAMESPACE
