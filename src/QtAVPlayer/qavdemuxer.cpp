/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavdemuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavsubtitlecodec_p.h"
#include "qavhwdevice_p.h"

#ifdef _MSC_VER
#include "qavplayer-config_p_windows.h"
#else
#include "qavplayer-config_p_linux.h"
#endif

#include <QtAVPlayer/qtavplayerglobal.h>
#include "qaviodevice_p.h"

#if QT_CONFIG(va_x11) && QT_CONFIG(opengl)
#include "qavhwdevice_vaapi_x11_glx_p.h"
#endif

#if QT_CONFIG(va_drm) && QT_CONFIG(egl)
#include "qavhwdevice_vaapi_drm_egl_p.h"
#endif

#if QT_CONFIG(vdpau)
#include "qavhwdevice_vdpau_p.h"
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include "qavhwdevice_videotoolbox_p.h"
#endif

#if defined(Q_OS_WIN)
#include "qavhwdevice_d3d11_p.h"
#include "qavhwdevice_dxva2_p.h"
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
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
#include <libavcodec/bsf.h>
#endif
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
    AVBSFContext *bsf_ctx = nullptr;

    bool abortRequest = false;
    mutable QMutex mutex;

    bool seekable = false;
    QList<QAVStream> streams;
    int currentAudioStreamIndex = -1;
    int currentVideoStreamIndex = -1;
    int currentSubtitleStreamIndex = -1;

    bool eof = false;
    QList<QAVPacket> packets;
    QString bsfs;
};

static int decode_interrupt_cb(void *ctx)
{
    auto d = reinterpret_cast<QAVDemuxerPrivate *>(ctx);
    QMutexLocker locker(&d->mutex);
    return d ? int(d->abortRequest) : 0;
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
    abort(false);
    unload();
}

void QAVDemuxer::abort(bool stop)
{
    Q_D(QAVDemuxer);
    d->abortRequest = stop;
}

static void setup_video_codec(AVStream *stream, QAVCodec *base)
{
    QList<QSharedPointer<QAVHWDevice>> devices;
    AVDictionary *opts = NULL;
    Q_UNUSED(opts);
    auto name = QGuiApplication::platformName();
    QAVVideoCodec &codec = *reinterpret_cast<QAVVideoCodec *>(base);

#if QT_CONFIG(va_x11) && QT_CONFIG(opengl)
    if (name == QLatin1String("xcb")) {
        devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VAAPI_X11_GLX));
        av_dict_set(&opts, "connection_type", "x11", 0);
    }
#endif
#if QT_CONFIG(va_drm) && QT_CONFIG(egl)
    if (name == QLatin1String("eglfs"))
        devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VAAPI_DRM_EGL));
#endif
#if QT_CONFIG(vdpau)
    devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VDPAU));
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (name == QLatin1String("cocoa") || name == QLatin1String("ios"))
        devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VideoToolbox));
#endif
#if defined(Q_OS_WIN) && defined(QT_AVPLAYER_USE_D3D11)
    if (name == QLatin1String("windows"))
        devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_D3D11));
#endif
#if defined(Q_OS_ANDROID)
    if (name == QLatin1String("android")) {
        devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_MediaCodec));
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

    if (devices.isEmpty()) {
        if (!supported.isEmpty())
            qWarning() << name << ": none of the hardware accelerations was implemented";
        return;
    }

    AVBufferRef *hw_device_ctx = nullptr;
    for (auto &device : devices) {
        auto deviceName = av_hwdevice_get_type_name(device->type());
        qDebug() << "Creating hardware device context:" << deviceName;
        if (av_hwdevice_ctx_create(&hw_device_ctx, device->type(), nullptr, opts, 0) >= 0) {
            qDebug() << "Using hardware device context:" << deviceName;
            codec.avctx()->hw_device_ctx = hw_device_ctx;
            codec.avctx()->pix_fmt = device->format();
            codec.setDevice(device);
            break;
        }
        av_buffer_unref(&hw_device_ctx);
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
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 0, 0)
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
#endif
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
        if (item.isEmpty())
            continue;
        if (item[0] == QLatin1Char('i'))
            input = item.mid(1).trimmed();
        else if (item[0] == QLatin1Char('f'))
            format = item.mid(1).trimmed();
    }

    parsed.input = input;
    parsed.format = format;

    return parsed;
}

static int init_output_bsfs(AVBSFContext *ctx, AVStream *st)
{
    if (!ctx)
        return 0;

    int ret = avcodec_parameters_copy(ctx->par_in, st->codecpar);
    if (ret < 0)
        return ret;

    ctx->time_base_in = st->time_base;

    ret = av_bsf_init(ctx);
    if (ret < 0) {
        qWarning() << "Error initializing bitstream filter:" << ctx->filter->name;
        return ret;
    }

    ret = avcodec_parameters_copy(st->codecpar, ctx->par_out);
    if (ret < 0)
        return ret;

    st->time_base = ctx->time_base_out;
    return 0;
}

static int apply_bsf(const QString &bsf, AVFormatContext *ctx, AVBSFContext *&bsf_ctx)
{
    int ret = !bsf.isEmpty() ? av_bsf_list_parse_str(bsf.toUtf8().constData(), &bsf_ctx) : 0;
    if (ret < 0) {
        qWarning() << "Error parsing bitstream filter sequence:" << bsf;
        return ret;
    }

    for (std::size_t i = 0; i < ctx->nb_streams; ++i) {
        switch (ctx->streams[i]->codecpar->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
            case AVMEDIA_TYPE_AUDIO:
                ret = init_output_bsfs(bsf_ctx, ctx->streams[i]);
                break;
            default:
                break;
        }
    }

    return ret;
}

int QAVDemuxer::load(const QString &url, QAVIODevice *dev)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);

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
    locker.unlock();
    int ret = avformat_open_input(&d->ctx, parsed.input.toUtf8().constData(), inputFormat, nullptr);
    if (ret < 0)
        return ret;

    ret = avformat_find_stream_info(d->ctx, NULL);
    if (ret < 0)
        return ret;

    locker.relock();
    d->currentVideoStreamIndex = av_find_best_stream(d->ctx, AVMEDIA_TYPE_VIDEO,
                                                     d->currentVideoStreamIndex, -1, nullptr, 0);
    d->currentAudioStreamIndex = av_find_best_stream(d->ctx, AVMEDIA_TYPE_AUDIO,
                                                     d->currentAudioStreamIndex,
                                                     d->currentVideoStreamIndex,
                                                     nullptr, 0);
    d->currentSubtitleStreamIndex = av_find_best_stream(d->ctx, AVMEDIA_TYPE_SUBTITLE,
                                                        d->currentSubtitleStreamIndex,
                                                        d->currentAudioStreamIndex >= 0 ? d->currentAudioStreamIndex : d->currentVideoStreamIndex,
                                                        nullptr, 0);
    av_log_set_callback(log_callback);

    for (std::size_t i = 0; i < d->ctx->nb_streams; ++i) {
        if (!d->ctx->streams[i]->codecpar) {
            qWarning() << "Could not find codecpar";
            return AVERROR(EINVAL);
        }
        enum AVMediaType type = d->ctx->streams[i]->codecpar->codec_type;
        switch (type) {
            case AVMEDIA_TYPE_VIDEO:
                d->streams.push_back({ int(i), d->ctx->streams[i], new QAVVideoCodec });
                setup_video_codec(d->ctx->streams[i], d->streams.last().codec().data());
                break;
            case AVMEDIA_TYPE_AUDIO:
                d->streams.push_back({ int(i), d->ctx->streams[i], new QAVAudioCodec });
                if (!d->streams.last().codec()->open(d->ctx->streams[i]))
                    qWarning() << "Could not open audio codec for stream:" << i;
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                d->streams.push_back({ int(i), d->ctx->streams[i], new QAVSubtitleCodec });
                if (!d->streams.last().codec()->open(d->ctx->streams[i]))
                    qWarning() << "Could not open subtitle codec for stream:" << i;
                break;
            default:
                // Adding default stream
                d->streams.push_back({ int(i), d->ctx->streams[i], nullptr });
                break;
        }
    }

    d->seekable = d->ctx->iformat->read_seek || d->ctx->iformat->read_seek2;
    if (d->ctx->pb)
        d->seekable |= bool(d->ctx->pb->seekable);

    if (!d->bsfs.isEmpty())
        return apply_bsf(d->bsfs, d->ctx, d->bsf_ctx);

    return 0;
}

QAVStream QAVDemuxer::stream(int index) const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return index >= 0 && index < d->streams.size()
           ? d->streams[index]
           : QAVStream{};
}

QList<QAVStream> QAVDemuxer::videoStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    QList<QAVStream> result;
    for (auto &stream : d->streams) {
        if (stream.stream()->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            result.push_back(stream);
    }

    return result;
}

QAVStream QAVDemuxer::videoStream() const
{
    Q_D(const QAVDemuxer);
    return stream(d->currentVideoStreamIndex);
}

bool QAVDemuxer::setVideoStream(const QAVStream &stream)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (stream.index() >= 0 && stream.index() < d->streams.size()
        && d->currentVideoStreamIndex != stream.index()
        && d->streams[stream.index()].stream()->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        d->currentVideoStreamIndex = stream.index();
        return true;
    }

    return false;
}

QList<QAVStream> QAVDemuxer::audioStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    QList<QAVStream> result;
    for (auto &stream : d->streams) {
        if (stream.stream()->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            result.push_back(stream);
    }

    return result;
}

QAVStream QAVDemuxer::audioStream() const
{
    Q_D(const QAVDemuxer);
    return stream(d->currentAudioStreamIndex);
}

bool QAVDemuxer::setAudioStream(const QAVStream &stream)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (stream.index() >= 0 && stream.index() < d->streams.size()
        && d->currentAudioStreamIndex != stream.index()
        && d->streams[stream.index()].stream()->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
        d->currentAudioStreamIndex = stream.index();
        return true;
    }

    return false;
}

QList<QAVStream> QAVDemuxer::subtitleStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    QList<QAVStream> result;
    for (auto &stream : d->streams) {
        if (stream.stream()->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
            result.push_back(stream);
    }

    return result;
}

QAVStream QAVDemuxer::subtitleStream() const
{
    Q_D(const QAVDemuxer);
    return stream(d->currentSubtitleStreamIndex);
}

bool QAVDemuxer::setSubtitleStream(const QAVStream &stream)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (stream.index() >= 0 && stream.index() < d->streams.size()
        && d->currentSubtitleStreamIndex != stream.index()
        && d->streams[stream.index()].stream()->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE)
    {
        d->currentSubtitleStreamIndex = stream.index();
        return true;
    }

    return false;
}

void QAVDemuxer::unload()
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (d->ctx) {
        avformat_close_input(&d->ctx);
        avformat_free_context(d->ctx);
    }
    d->ctx = nullptr;
    d->eof = false;
    d->abortRequest = 0;
    d->currentVideoStreamIndex = -1;
    d->currentAudioStreamIndex = -1;
    d->currentSubtitleStreamIndex = -1;
    d->streams.clear();
    av_bsf_free(&d->bsf_ctx);
    d->bsf_ctx = nullptr;
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
    QMutexLocker locker(&d->mutex);
    if (!d->packets.isEmpty())
        return d->packets.takeFirst();

    if (!d->ctx || d->eof)
        return {};

    QAVPacket pkt;
    locker.unlock();
    int ret = av_read_frame(d->ctx, pkt.packet());
    if (ret < 0) {
        if (ret == AVERROR_EOF || avio_feof(d->ctx->pb)) {
            locker.relock();
            d->eof = true;
            locker.unlock();
        }
    }

    locker.relock();

    AVStream *stream = nullptr;
    if (pkt.packet()->stream_index == d->currentVideoStreamIndex)
        stream = d->ctx->streams[d->currentVideoStreamIndex];
    else if (pkt.packet()->stream_index == d->currentAudioStreamIndex) 
        stream = d->ctx->streams[d->currentAudioStreamIndex];
    else if (pkt.packet()->stream_index == d->currentSubtitleStreamIndex)
        stream = d->ctx->streams[d->currentSubtitleStreamIndex];

    if (stream != nullptr)
        pkt.setTimeBase(stream->time_base);

    if (d->bsf_ctx) {
        ret = av_bsf_send_packet(d->bsf_ctx, d->eof ? NULL : pkt.packet());
        if (ret >= 0) {
            while ((ret = av_bsf_receive_packet(d->bsf_ctx, pkt.packet())) >= 0)
                d->packets.append(pkt);
        }
        if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN)) {
            qWarning() << "Error applying bitstream filters to an output:" << ret;
            return {};
        }
    } else {
        d->packets.append(pkt);
    }

    return !d->packets.isEmpty() ? d->packets.takeFirst() : QAVPacket{};
}

static bool decode_packet(const QAVDemuxer &demuxer, const QAVPacket &pkt, QAVFrame *frame, QAVSubtitleFrame *subtitle)
{
    auto s = demuxer.stream(pkt.packet()->stream_index);
    if (!s)
        return false;

    bool result = false;
    switch (s.stream()->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_AUDIO: {
            if (frame) {
                auto c = reinterpret_cast<QAVFrameCodec *>(s.codec().data());
                result = c->decode(pkt.packet(), frame->frame());
                if (result)
                    frame->setStream(s);
            }
        } break;
        case AVMEDIA_TYPE_SUBTITLE: {
            if (subtitle) {
                auto c = reinterpret_cast<QAVSubtitleCodec *>(s.codec().data());
                result = c->decode(pkt.packet(), subtitle->subtitle());
                if (result)
                    subtitle->setStream(s);
            }
        } break;
        default:
            break;
    }

    return result;
}

bool QAVDemuxer::decode(const QAVPacket &pkt, QAVSubtitleFrame &frame) const
{
    return decode_packet(*this, pkt, nullptr, &frame);
}

bool QAVDemuxer::decode(const QAVPacket &pkt, QAVFrame &frame) const
{
    return decode_packet(*this, pkt, &frame, nullptr);
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
    if (d->currentVideoStreamIndex < 0)
        return 1 / 24.0;

    AVRational fr = av_guess_frame_rate(d->ctx, d->ctx->streams[d->currentVideoStreamIndex], NULL);
    return fr.num && fr.den ? av_q2d({fr.den, fr.num}) : 0.0;
}

AVRational QAVDemuxer::frameRate() const
{
    Q_D(const QAVDemuxer);
    return av_guess_frame_rate(d->ctx, d->ctx->streams[d->currentVideoStreamIndex], NULL);
}

QMap<QString, QString> QAVDemuxer::metadata() const
{
    Q_D(const QAVDemuxer);
    QMap<QString, QString> result;
    if (d->ctx == nullptr)
        return result;

    AVDictionaryEntry *tag = nullptr;
    while ((tag = av_dict_get(d->ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
        result[QString::fromUtf8(tag->key)] = QString::fromUtf8(tag->value);

    return result;
}

QString QAVDemuxer::bitstreamFilter() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->bsfs;
}

int QAVDemuxer::applyBitstreamFilter(const QString &bsfs)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    d->bsfs = bsfs;
    int ret = 0;
    if (d->ctx) {
        av_bsf_free(&d->bsf_ctx);
        d->bsf_ctx = nullptr;
        ret = apply_bsf(d->bsfs, d->ctx, d->bsf_ctx);
    }
    return ret;
}

QStringList QAVDemuxer::supportedBitstreamFilters()
{
    QStringList result;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 0, 0)
    const AVBitStreamFilter *bsf = NULL;
    void *opaque = NULL;

    while ((bsf = av_bsf_iterate(&opaque)))
        result.append(QString::fromUtf8(bsf->name));
#endif
    return result;
}

QT_END_NAMESPACE
