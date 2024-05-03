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
#include "qaviodevice.h"
#include <QtAVPlayer/qtavplayerglobal.h>

#if defined(QT_AVPLAYER_VA_X11) && QT_CONFIG(opengl)
#include "qavhwdevice_vaapi_x11_glx_p.h"
#endif

#if defined(QT_AVPLAYER_VA_DRM) && QT_CONFIG(egl)
#include "qavhwdevice_vaapi_drm_egl_p.h"
#endif

#if defined(QT_AVPLAYER_VDPAU)
#include "qavhwdevice_vdpau_p.h"
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

#include <QDir>
#include <QSharedPointer>
#include <QMutexLocker>
#include <atomic>
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

    std::atomic_bool abortRequest = false;
    mutable QMutex mutex;

    bool seekable = false;
    QList<QAVStream> availableStreams;
    QList<QAVStream> currentVideoStreams;
    QList<QAVStream> currentAudioStreams;
    QList<QAVStream> currentSubtitleStreams;
    QList<QAVStream::Progress> progress;
    QString inputFormat;
    QString inputVideoCodec;
    QMap<QString, QString> inputOptions;

    bool eof = false;
    QList<QAVPacket> packets;
    QString bsfs;
};

static int decode_interrupt_cb(void *ctx)
{
    auto d = reinterpret_cast<QAVDemuxerPrivate *>(ctx);
    return d ? int(d->abortRequest) : 0;
}

QAVDemuxer::QAVDemuxer()
    : d_ptr(new QAVDemuxerPrivate(this))
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

static int setup_video_codec(const QString &inputVideoCodec, AVStream *stream, QAVVideoCodec &codec)
{
    const AVCodec *videoCodec = nullptr;
    if (!inputVideoCodec.isEmpty()) {
        qDebug() << "Loading: -vcodec" << inputVideoCodec;
        videoCodec = avcodec_find_decoder_by_name(inputVideoCodec.toUtf8().constData());
        if (!videoCodec) {
            qWarning() << "Could not find decoder:" << inputVideoCodec;
            return AVERROR(EINVAL);
        }
    }

    if (videoCodec)
        codec.setCodec(videoCodec);

    QList<QSharedPointer<QAVHWDevice>> devices;
    AVDictionary *opts = NULL;
    Q_UNUSED(opts);

#if defined(QT_AVPLAYER_VA_X11) && QT_CONFIG(opengl)
    devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VAAPI_X11_GLX));
    av_dict_set(&opts, "connection_type", "x11", 0);
#endif
#if defined(QT_AVPLAYER_VDPAU)
    devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VDPAU));
#endif
#if defined(QT_AVPLAYER_VA_DRM) && QT_CONFIG(egl)
    devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VAAPI_DRM_EGL));
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_VideoToolbox));
#endif
#if defined(Q_OS_WIN)
    devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_D3D11));
#endif
#if defined(Q_OS_ANDROID)
    devices.append(QSharedPointer<QAVHWDevice>(new QAVHWDevice_MediaCodec));
    if (!codec.codec())
        codec.setCodec(avcodec_find_decoder_by_name("h264_mediacodec"));
    auto vm = QtAndroidPrivate::javaVM();
    av_jni_set_java_vm(vm, NULL);
#endif

    const bool ignoreHW = qEnvironmentVariableIsSet("QT_AVPLAYER_NO_HWDEVICE");
    if (!ignoreHW) {
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

    // Open codec after hwdevices
    if (!codec.open(stream)) {
        qWarning() << "Could not open video codec for stream";
        return AVERROR(EINVAL);
    }

    return 0;
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

QStringList QAVDemuxer::supportedVideoCodecs()
{
    static QStringList values;
    if (values.isEmpty()) {
        const AVCodec *c = nullptr;
        void *it = nullptr;
        while ((c = av_codec_iterate(&it))) {
            if (!av_codec_is_decoder(c) || c->type != AVMEDIA_TYPE_VIDEO)
                continue;
            values.append(QString::fromLatin1(c->name));
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
    if (!d->inputFormat.isEmpty()) {
        qDebug() << "Loading: -f" << d->inputFormat;
        inputFormat = av_find_input_format(d->inputFormat.toUtf8().constData());
        if (!inputFormat) {
            qWarning() << "Could not find input format:" << d->inputFormat;
            return AVERROR(EINVAL);
        }
    }

    AVDictionary *opts = nullptr;
    for (const auto & key: d->inputOptions.keys())
        av_dict_set(&opts, key.toUtf8().constData(), d->inputOptions[key].toUtf8().constData(), 0);
    locker.unlock();
    int ret = avformat_open_input(&d->ctx, url.toUtf8().constData(), inputFormat, &opts);
    if (ret < 0)
        return ret;

    ret = avformat_find_stream_info(d->ctx, NULL);
    if (ret < 0)
        return ret;

    locker.relock();
    av_log_set_callback(log_callback);

#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(59, 8, 0)
    d->seekable = d->ctx->iformat->read_seek || d->ctx->iformat->read_seek2;
    if (d->ctx->pb)
        d->seekable |= bool(d->ctx->pb->seekable);
#else
    // TODO: Search and implement replacement function for seek
    d->seekable = true;
#endif

    ret = resetCodecs();
    if (ret < 0)
        return ret;

    const int videoStreamIndex = av_find_best_stream(
        d->ctx,
        AVMEDIA_TYPE_VIDEO,
        -1,
        -1,
        nullptr,
        0);
    if (videoStreamIndex >= 0)
        d->currentVideoStreams.push_back(d->availableStreams[videoStreamIndex]);

    const int audioStreamIndex = av_find_best_stream(
        d->ctx,
        AVMEDIA_TYPE_AUDIO,
        -1,
        videoStreamIndex,
        nullptr,
        0);
    if (audioStreamIndex >= 0)
        d->currentAudioStreams.push_back(d->availableStreams[audioStreamIndex]);

    const int subtitleStreamIndex = av_find_best_stream(
        d->ctx,
        AVMEDIA_TYPE_SUBTITLE,
        -1,
        audioStreamIndex >= 0 ? audioStreamIndex : videoStreamIndex,
        nullptr,
        0);
    if (subtitleStreamIndex >= 0)
        d->currentSubtitleStreams.push_back(d->availableStreams[subtitleStreamIndex]);

    if (ret < 0)
        return ret;

    if (!d->bsfs.isEmpty())
        return apply_bsf(d->bsfs, d->ctx, d->bsf_ctx);

    return 0;
}

int QAVDemuxer::resetCodecs()
{
    Q_D(QAVDemuxer);
    int ret = 0;
    for (std::size_t i = 0; i < d->ctx->nb_streams && ret >= 0; ++i) {
        if (!d->ctx->streams[i]->codecpar) {
            qWarning() << "Could not find codecpar";
            return AVERROR(EINVAL);
        }
        const AVMediaType type = d->ctx->streams[i]->codecpar->codec_type;
        switch (type) {
            case AVMEDIA_TYPE_VIDEO:
            {
                QSharedPointer<QAVCodec> codec(new QAVVideoCodec);
                d->availableStreams.push_back({ int(i), d->ctx, codec });
                ret = setup_video_codec(d->inputVideoCodec, d->ctx->streams[i], *static_cast<QAVVideoCodec *>(codec.data()));
            } break;
            case AVMEDIA_TYPE_AUDIO:
                d->availableStreams.push_back({ int(i), d->ctx, QSharedPointer<QAVCodec>(new QAVAudioCodec) });
                if (!d->availableStreams.last().codec()->open(d->ctx->streams[i]))
                    qWarning() << "Could not open audio codec for stream:" << i;
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                d->availableStreams.push_back({ int(i), d->ctx, QSharedPointer<QAVCodec>(new QAVSubtitleCodec) });
                if (!d->availableStreams.last().codec()->open(d->ctx->streams[i]))
                    qWarning() << "Could not open subtitle codec for stream:" << i;
                break;
            default:
                // Adding default stream
                d->availableStreams.push_back({ int(i), d->ctx, nullptr });
                break;
        }
        auto &s = d->availableStreams[int(i)];
        d->progress.push_back({ s.duration(), s.framesCount(), s.frameRate() });
    }

    return ret;
}

static bool findStream(
    const QList<QAVStream> &streams,
    int index)
{
    for (const auto &stream: streams) {
        if (index == stream.index())
            return true;
    }
    return false;
}

AVMediaType QAVDemuxer::currentCodecType(int index) const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    // TODO:
    if (findStream(d->currentVideoStreams, index))
        return AVMEDIA_TYPE_VIDEO;
    if (findStream(d->currentAudioStreams, index))
        return AVMEDIA_TYPE_AUDIO;
    if (findStream(d->currentSubtitleStreams, index))
        return AVMEDIA_TYPE_SUBTITLE;
    return AVMEDIA_TYPE_UNKNOWN;
}

static QList<QAVStream> availableStreamsByType(
    const QList<QAVStream> &streams,
    AVMediaType type)
{
    QList<QAVStream> ret;
    for (auto &stream : streams) {
        if (stream.stream()->codecpar->codec_type == type)
            ret.push_back(stream);
    }

    return ret;
}

QList<QAVStream> QAVDemuxer::availableVideoStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return availableStreamsByType(d->availableStreams, AVMEDIA_TYPE_VIDEO);
}

QList<QAVStream> QAVDemuxer::currentVideoStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->currentVideoStreams;
}

static bool setCurrentStreams(
    const QList<QAVStream> &streams,
    const QList<QAVStream> &availableStreams,
    AVMediaType type,
    QList<QAVStream> &currentStreams)
{
    QList<QAVStream> ret;
    for (const auto &stream: streams) {
        if (stream.index() >= 0
            && stream.index() < availableStreams.size()
            && availableStreams[stream.index()].stream()->codecpar->codec_type == type)
        {
            ret.push_back(availableStreams[stream.index()]);
        }
    }
    if (!ret.isEmpty() || streams.isEmpty()) {
        currentStreams = ret;
        return true;
    }

    return false;
}

bool QAVDemuxer::setVideoStreams(const QList<QAVStream> &streams)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return setCurrentStreams(
        streams,
        d->availableStreams,
        AVMEDIA_TYPE_VIDEO,
        d->currentVideoStreams);
}

QList<QAVStream> QAVDemuxer::availableAudioStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return availableStreamsByType(
        d->availableStreams,
        AVMEDIA_TYPE_AUDIO);
}

QList<QAVStream> QAVDemuxer::currentAudioStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->currentAudioStreams;
}

bool QAVDemuxer::setAudioStreams(const QList<QAVStream> &streams)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return setCurrentStreams(
        streams,
        d->availableStreams,
        AVMEDIA_TYPE_AUDIO,
        d->currentAudioStreams);
}

QList<QAVStream> QAVDemuxer::availableSubtitleStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return availableStreamsByType(
        d->availableStreams,
        AVMEDIA_TYPE_SUBTITLE);
}

QList<QAVStream> QAVDemuxer::currentSubtitleStreams() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->currentSubtitleStreams;
}

bool QAVDemuxer::setSubtitleStreams(const QList<QAVStream> &streams)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return setCurrentStreams(
        streams,
        d->availableStreams,
        AVMEDIA_TYPE_SUBTITLE,
        d->currentSubtitleStreams);
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
    d->currentVideoStreams.clear();
    d->currentAudioStreams.clear();
    d->currentSubtitleStreams.clear();
    d->availableStreams.clear();
    d->progress.clear();
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
    {
        QMutexLocker locker(&d->mutex);
        if (!d->packets.isEmpty())
            return d->packets.takeFirst();

        if (!d->ctx || d->eof)
            return {};
    }

    QAVPacket pkt;
    bool eof = d->eof;
    int ret = av_read_frame(d->ctx, pkt.packet());
    if (ret < 0) {
        if (ret == AVERROR_EOF || avio_feof(d->ctx->pb)) {
            eof = true;
        } else {
            qDebug() << "av_read_frame: unexpected result:" << ret;
            return {};
        }
    }
    {
        QMutexLocker locker(&d->mutex);
        d->eof = eof;
        if (pkt.packet()->stream_index < d->availableStreams.size())
            pkt.setStream(d->availableStreams[pkt.packet()->stream_index]);
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
            return !d->packets.isEmpty() ? d->packets.takeFirst() : QAVPacket{};
        }
    }
    return pkt;
}

void QAVDemuxer::decode(const QAVPacket &pkt, QList<QAVFrame> &frames) const
{
    if (!pkt.stream())
        return;
    int sent = 0;
    do {
        sent = pkt.send();
        // AVERROR(EAGAIN): input is not accepted in the current state - user must read output with avcodec_receive_frame()
        // (once all output is read, the packet should be resent, and the call will not fail with EAGAIN)
        if (sent < 0 && sent != AVERROR(EAGAIN))
            return;

        while (true) {
            QAVFrame frame;
            frame.setStream(pkt.stream());
            // AVERROR(EAGAIN): output is not available in this state - user must try to send new input
            int received = frame.receive();
            if (received < 0)
                break;
            frames.push_back(frame);
        }
    } while (sent == AVERROR(EAGAIN));
}

void QAVDemuxer::decode(const QAVPacket &pkt, QList<QAVSubtitleFrame> &frames) const
{
    if (!pkt.stream())
        return;
    int sent = pkt.send();
    if (sent < 0 && sent != AVERROR(EAGAIN))
        return;

    QAVSubtitleFrame frame;
    frame.setStream(pkt.stream());
    if (frame.receive() >= 0)
        frames.push_back(frame);
}

void QAVDemuxer::flushCodecBuffers()
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    for (auto &s: d->availableStreams) {
        auto c = s.codec();
        if (c)
            c->flushBuffers();
    }
}

bool QAVDemuxer::seekable() const
{
    return d_func()->seekable;
}

int QAVDemuxer::seek(double sec)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    if (!d->ctx || !d->seekable)
        return AVERROR(EINVAL);

    d->eof = false;
    locker.unlock();

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
    if (d->currentVideoStreams.isEmpty())
        return 0.0;
    // TODO:
    double ret = std::numeric_limits<double>::max();
    for (const auto &stream: d->currentVideoStreams) {
        AVRational fr = av_guess_frame_rate(d->ctx, d->ctx->streams[stream.index()], NULL);
        double rate = fr.num && fr.den ? av_q2d({fr.den, fr.num}) : 0.0;
        if (rate < ret)
            ret = rate;
    }

    return ret;
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

QString QAVDemuxer::inputFormat() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->inputFormat;
}

void QAVDemuxer::setInputFormat(const QString &format)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    d->inputFormat = format;
}

QString QAVDemuxer::inputVideoCodec() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->inputVideoCodec;
}

void QAVDemuxer::setInputVideoCodec(const QString &codec)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    d->inputVideoCodec = codec;
}

QMap<QString, QString> QAVDemuxer::inputOptions() const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    return d->inputOptions;
}

void QAVDemuxer::setInputOptions(const QMap<QString, QString> &opts)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    d->inputOptions = opts;
}

void QAVDemuxer::onFrameSent(const QAVStreamFrame &frame)
{
    Q_D(QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    int index = frame.stream().index();
    if (index >= 0 && index < d->progress.size())
        d->progress[index].onFrameSent(frame.pts());
}

bool QAVDemuxer::isMasterStream(const QAVStream &stream) const
{
    auto s = stream.stream();
    switch (s->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            return s->disposition != AV_DISPOSITION_ATTACHED_PIC;
        case AVMEDIA_TYPE_AUDIO:
            // Check if there are any video streams available
            for (const auto &vs: currentVideoStreams()) {
                if (vs.stream()->disposition != AV_DISPOSITION_ATTACHED_PIC)
                    return false;
            }
            return true;
        default:
            Q_ASSERT(false);
            return false;
    }
}

QAVStream::Progress QAVDemuxer::progress(const QAVStream &s) const
{
    Q_D(const QAVDemuxer);
    QMutexLocker locker(&d->mutex);
    int index = s.index();
    if (index >= 0 && index < d->progress.size())
        return d->progress[index];
    return {};
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
