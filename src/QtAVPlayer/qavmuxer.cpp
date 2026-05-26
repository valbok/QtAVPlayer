/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavmuxer.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavsubtitlecodec_p.h"
#include "qavvideoframe.h"
#include "qavformatcontext_p.h"

#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <QDebug>

extern "C" {
#if defined(QT_AVPLAYER_LIBASS)
    #include <ass/ass.h>
#endif
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

av_always_inline char *err2str(int errnum)
{
    thread_local char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

QT_BEGIN_NAMESPACE

class QAVMuxerPrivate : public QObject
{
    Q_DECLARE_PUBLIC(QAVMuxer)
public:
    QAVMuxerPrivate(QAVMuxer *q)
        : q_ptr(q)
    {
    }

    int outputStreamIndex(const QAVStream &stream, Locker &locker) const;

    QAVMuxer *q_ptr = nullptr;
    QList<QAVStream> inputStreams;
    // Mapping between input frame and output stream index in ctx->streams.
    // Allows to mux frames or packets from different sources.
    QMap<AVStream *, int> outputStreams;
    QString filename;
    QSharedPointer<QAVFormatContext> ctx;
    bool loaded = false;
    mutable QMutex mutex;
};

int QAVMuxerPrivate::outputStreamIndex(const QAVStream &stream, Locker &) const
{
    if (!stream)
        return -1;
    auto it = outputStreams.find(stream.stream());
    if (it == outputStreams.end())
        return -1;
    return *it;
}

class QAVMuxerFramesPrivate : public QAVMuxerPrivate
{
    Q_DECLARE_PUBLIC(QAVMuxerFrames)
public:
    QAVMuxerFramesPrivate(QAVMuxerFrames *q)
        : QAVMuxerPrivate(q)
    {
    }

    QMap<int, QAVStream> encStreams;
    std::unique_ptr<QThread> workerThread;
    QList<QAVFrame> frames;
    QWaitCondition cond;
    bool quit = false;

    void doWork();
};

void QAVMuxerFramesPrivate::doWork()
{
    Q_Q(QAVMuxerFrames);
    QMutexLocker locker(&mutex);
    while (!quit) {
        if (frames.isEmpty()) {
            cond.wait(&mutex);
            if (quit || frames.isEmpty())
                break;
        }
        auto frame = frames.takeFirst();
        if (!frame)
            continue;
        int index = outputStreamIndex(frame.stream(), locker);
        // Ignore wrong frames
        if (index < 0)
            continue;
        q->write(frame, index, locker);
    }
}

QAVMuxer::QAVMuxer()
    : QAVMuxer(*new QAVMuxerPrivate(this))
{
}

QAVMuxer::QAVMuxer(QAVMuxerPrivate &d)
    : d_ptr(&d)
{
}

QAVMuxer::~QAVMuxer()
{
}

void QAVMuxer::unload()
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    reset(locker);
}

void QAVMuxer::reset(Locker &locker)
{
    Q_D(QAVMuxer);
    close(locker);
    if (d->ctx && d->ctx->ctx() && !(d->ctx->ctx()->oformat->flags & AVFMT_NOFILE))
        avio_closep(&d->ctx->ctx()->pb);
    d->ctx = nullptr;
    d->inputStreams.clear();
    d->outputStreams.clear();
    d->filename.clear();
}

void QAVMuxer::close(Locker &locker)
{
    Q_D(QAVMuxer);
    if (d->ctx && d->ctx->ctx() && d->loaded) {
        flushFrames(locker);
        av_write_trailer(d->ctx->ctx());
    }
    d->loaded = false;
}

int QAVMuxer::load(const QList<QAVStream> &streams, const QString &filename)
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    reset(locker);
    d->ctx = QAVFormatContext::alloc(filename);
    if (!d->ctx)
        return AVERROR(EINVAL);
    d->inputStreams = streams;
    d->filename = filename;
    int ret = initMuxer(locker);
    if (ret < 0)
        return ret;

    if (!(d->ctx->ctx()->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&d->ctx->ctx()->pb, filename.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            qWarning() << "Could not open output file:" << filename << ":"<< err2str(ret);
            return ret;
        }
    }

    // Init muxer, write output file header
    ret = avformat_write_header(d->ctx->ctx(), nullptr);
    if (ret < 0) {
        qWarning() << filename << ": Failed avformat_write_header:" << err2str(ret);
        return ret;
    }
    init(locker);
    d->loaded = true;
    return 0;
}

int QAVMuxer::initMuxer(Locker &locker)
{
    Q_D(QAVMuxer);
    int ret = 0;
    for (int i = 0; i < d->inputStreams.size(); ++i) {
        auto &stream = d->inputStreams[i];
        auto codec = stream.codec();
        if (!stream.codec())
            return AVERROR(EINVAL);
        auto dec_ctx = codec->avctx();
        auto out_stream = avformat_new_stream(d->ctx->ctx(), NULL);
        if (!out_stream) {
            qWarning() << "Failed allocating output stream";
            return AVERROR_UNKNOWN;
        }

        const auto pix_fmt_desc = av_pix_fmt_desc_get(dec_ctx->pix_fmt);
        qDebug() << "[" << d->filename << "][" << stream.index() << "][" <<
            av_get_media_type_string(dec_ctx->codec_type) << "]: Using" <<
            stream.codec()->codec()->name << ", codec_id" << dec_ctx->codec_id <<
            ", pix_fmt:" << dec_ctx->pix_fmt << (pix_fmt_desc ? pix_fmt_desc->name : "");

        ret = initMuxer(stream, i, d->ctx->ctx()->streams[i], locker);
        if (ret < 0)
            return ret;
        d->outputStreams[stream.stream()] = i;
    }
    av_dump_format(d->ctx->ctx(), 0, d->filename.toUtf8().constData(), 1);
    return 0;
}

QAVMuxerPackets::~QAVMuxerPackets()
{
    unload();
}

void QAVMuxerPackets::init(Locker &)
{
}

int QAVMuxerPackets::initMuxer(const QAVStream &stream, int, AVStream *out_stream, Locker &)
{
    auto in_stream = stream.stream();
    int ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    if (ret < 0) {
        qWarning() << "Failed to copy codec parameters:" << err2str(ret);
        return ret;
    }
    out_stream->codecpar->codec_tag = 0;
    return 0;
}

int QAVMuxerPackets::write(const QAVPacket &packet)
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    if (!d->loaded || !packet.stream())
        return 0;
    int index = d->outputStreamIndex(packet.stream(), locker);
    if (index < 0)
        return AVERROR(EINVAL);
    return write(packet, index, locker);
}

int QAVMuxerPackets::write(QAVPacket packet, int streamIndex, Locker &)
{
    Q_D(QAVMuxer);
    auto stream = packet.stream();
    AVPacket *enc_pkt = nullptr;
    if (stream) {
        auto in_stream = stream.stream();
        Q_ASSERT(streamIndex < static_cast<int>(d->ctx->ctx()->nb_streams));
        auto out_stream = d->ctx->ctx()->streams[streamIndex];
        enc_pkt = packet.packet();
        enc_pkt->stream_index = streamIndex;
        av_packet_rescale_ts(enc_pkt, in_stream->time_base, out_stream->time_base);
        enc_pkt->pos = -1;
    }
    return av_interleaved_write_frame(d->ctx->ctx(), enc_pkt);
}

int QAVMuxer::flush()
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    if (!d->loaded)
        return 0;
    return flushFrames(locker);
}

int QAVMuxerPackets::flushFrames(Locker &locker)
{
    Q_D(QAVMuxer);
    for (unsigned i = 0; i < d->ctx->ctx()->nb_streams; ++i) {
        // no flushing for subtitles
        bool isSub = d->ctx->ctx()->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE;
        if (isSub)
            continue;
        int ret = write(QAVPacket(), i, locker);
        if (ret < 0 && ret != AVERROR_EOF) {
            qWarning() << d->filename << ": Could not flush:" << err2str(ret);
            return ret;
        }
    }
    return 0;
}

QAVMuxerFrames::QAVMuxerFrames()
    : QAVMuxer(*new QAVMuxerFramesPrivate(this))
{
}

QAVMuxerFrames::~QAVMuxerFrames()
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    flushFrames(locker);
    stop(locker);
    reset(locker);
}

void QAVMuxerFrames::enqueue(const QAVFrame &frame)
{
    Q_D(QAVMuxerFrames);
    {
        QMutexLocker locker(&d->mutex);
        if (!d->loaded)
            return;
        d->frames.push_back(frame);
    }
    d->cond.wakeAll();
}

size_t QAVMuxerFrames::size() const
{
    Q_D(const QAVMuxerFrames);
    QMutexLocker locker(&d->mutex);
    return d->frames.size();
}

void QAVMuxerFrames::init(Locker &)
{
    Q_D(QAVMuxerFrames);
    d->quit = false;
    d->workerThread.reset(new QThread);
    QObject::connect(d->workerThread.get(), &QThread::started, d, &QAVMuxerFramesPrivate::doWork, Qt::DirectConnection);
    d->workerThread->start();
}

int QAVMuxerFrames::initMuxer(const QAVStream &stream, int index, AVStream *out_stream, Locker &)
{
    Q_D(QAVMuxerFrames);
    auto in_stream = stream.stream();
    auto dec_ctx = stream.codec()->avctx();
    // Transcoding to same codec
    auto encoder = avcodec_find_encoder(dec_ctx->codec_id);
    if (!encoder) {
        qWarning() << "Encoder not found:" << stream.codec()->codec()->name;
        return AVERROR_INVALIDDATA;
    }

    AVCodecContext *enc_ctx = nullptr;
    QSharedPointer<QAVCodec> codec;
    int ret = 0;

    switch (dec_ctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO: {
            // pix_fmt might be not supported by the encoder, f.e. vdpau
            // so falling back to software pixel format
            if (dec_ctx->sw_pix_fmt != AV_PIX_FMT_NONE)
                dec_ctx->pix_fmt = dec_ctx->sw_pix_fmt;
            codec.reset(new QAVVideoCodec(encoder));
            enc_ctx = codec->avctx();
            enc_ctx->height = dec_ctx->height;
            enc_ctx->width = dec_ctx->width;
            enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
            enc_ctx->pix_fmt = dec_ctx->pix_fmt;
            enc_ctx->time_base = in_stream->time_base;
            if (d->ctx->ctx()->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            if (!codec->open(out_stream)) {
                qWarning() << stream.index() << ": Cannot open encoder:" << encoder->name;
                return AVERROR_UNKNOWN;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                qWarning() << stream.index() << ": Failed to copy encoder parameters to output stream:" << err2str(ret);
                return ret;
            }
            out_stream->time_base = enc_ctx->time_base;
            d->encStreams[index] = { index, d->ctx, codec };
            break;
        }
        case AVMEDIA_TYPE_AUDIO: {
            codec.reset(new QAVAudioCodec(encoder));
            enc_ctx = codec->avctx();
            enc_ctx->sample_rate = dec_ctx->sample_rate;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
            ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
            if (ret < 0) {
                qWarning() << stream.index() << ": Failed av_channel_layout_copy:" << err2str(ret);
                return ret;
            }
#endif
            enc_ctx->sample_fmt = dec_ctx->sample_fmt;
            enc_ctx->time_base = in_stream->time_base;
            if (d->ctx->ctx()->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            if (!codec->open(out_stream)) {
                qWarning() << stream.index() << ": Cannot open encoder:" << encoder->name;
                return AVERROR_UNKNOWN;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                qWarning() << stream.index() << ": Failed to copy encoder parameters to output stream:" << err2str(ret);
                return ret;
            }
            out_stream->time_base = enc_ctx->time_base;
            d->encStreams[index] = { index, d->ctx, codec };
            break;
        }
        case AVMEDIA_TYPE_SUBTITLE: {
            codec.reset(new QAVSubtitleCodec(encoder));
            enc_ctx = codec->avctx();
            enc_ctx->time_base = AV_TIME_BASE_Q;
            enc_ctx->height = dec_ctx->height;
            enc_ctx->width = dec_ctx->width;
            if (dec_ctx->subtitle_header) {
                // ASS code assumes this buffer is null terminated so add extra byte.
                enc_ctx->subtitle_header = (uint8_t*) av_mallocz(dec_ctx->subtitle_header_size + 1);
                if (!enc_ctx->subtitle_header)
                    return AVERROR(ENOMEM);
                memcpy(enc_ctx->subtitle_header, dec_ctx->subtitle_header,
                       dec_ctx->subtitle_header_size);
                enc_ctx->subtitle_header_size = dec_ctx->subtitle_header_size;
            }
            if (!codec->open(out_stream)) {
                qWarning() << stream.index() << ": Cannot open encoder: " << encoder->name;
                return AVERROR_UNKNOWN;
            }
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                qWarning() << stream.index() << ": Copying parameters for stream failed:" << err2str(ret);
                return ret;
            }
            out_stream->time_base = in_stream->time_base;
            d->encStreams[index] = { index, d->ctx, codec };
            break;
        }
        default:
            qWarning() << "Unexpected stream:" << dec_ctx->codec_type;
            return AVERROR_INVALIDDATA;
    }
    return 0;
}

int QAVMuxerFrames::write(const QAVFrame &frame)
{
    Q_D(QAVMuxerFrames);
    QMutexLocker locker(&d->mutex);
    if (!d->loaded)
        return 0;
    int index = d->outputStreamIndex(frame.stream(), locker);
    if (index < 0)
        return AVERROR(EINVAL);
    return write(frame, index, locker);
}

int QAVMuxerFrames::write(const QAVSubtitleFrame &frame)
{
    Q_D(QAVMuxerFrames);
    QMutexLocker locker(&d->mutex);
    if (!d->loaded)
        return 0;
    int index = d->outputStreamIndex(frame.stream(), locker);
    if (index < 0)
        return AVERROR(EINVAL);
    return write(frame, index, locker);
}

int QAVMuxerFrames::write(QAVFrame frame, int streamIndex, Locker &)
{
    Q_D(QAVMuxerFrames);
    auto &encStream = d->encStreams[streamIndex];
    auto enc_ctx = encStream.codec()->avctx();
    auto stream = encStream.stream();

    if (enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO && frame && frame.frame()->format != enc_ctx->pix_fmt) {
        qWarning() << av_pix_fmt_desc_get(AVPixelFormat(frame.frame()->format))->name
                   << "differs to encoder:" << av_pix_fmt_desc_get(enc_ctx->pix_fmt)->name;
        return AVERROR_INVALIDDATA;
    }

    // Set encoder stream to frame to send to
    frame.setStream(encStream);
    int sent = 0;
    do {
        sent = frame.send();
        if (sent < 0 && sent != AVERROR(EAGAIN))
            return sent;

        int wrote = 0;
        while (wrote >= 0) {
            QAVPacket pkt;
            pkt.setStream(encStream);
            int received = pkt.receive();
            if (received < 0)
                break;
            auto enc_pkt = pkt.packet();
            // Points in streams in d->encStreams
            enc_pkt->stream_index = streamIndex;
            av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, stream->time_base);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
            enc_pkt->time_base = stream->time_base;
#endif
            wrote = av_interleaved_write_frame(d->ctx->ctx(), enc_pkt);
        }
    } while (sent == AVERROR(EAGAIN));
    return 0;
}

int QAVMuxerFrames::write(QAVSubtitleFrame frame, int streamIndex, Locker &)
{
    Q_D(QAVMuxerFrames);
    auto &encStream = d->encStreams[streamIndex];
    auto enc_ctx = encStream.codec()->avctx();
    auto stream = encStream.stream();

    // Set encoder stream to frame to send to
    frame.setStream(encStream);
    int sent = frame.send();
    if (sent < 0 && sent != AVERROR(EAGAIN))
        return sent;
    QAVPacket pkt;
    pkt.setStream(encStream);
    int received = pkt.receive();
    if (received < 0)
        return received;
    auto enc_pkt = pkt.packet();
    enc_pkt->stream_index = streamIndex;
    av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, stream->time_base);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
    enc_pkt->time_base = stream->time_base;
#endif
    int wrote = av_interleaved_write_frame(d->ctx->ctx(), enc_pkt);
    return wrote;
}

void QAVMuxerFrames::stop(Locker &locker)
{
    Q_D(QAVMuxerFrames);
    d->quit = true;
    bool loaded = static_cast<bool>(d->workerThread);
    locker.unlock();
    d->cond.wakeAll();
    if (loaded) {
        d->workerThread->quit();
        d->workerThread->wait();
    }
    locker.relock();
    d->workerThread.reset();
}

void QAVMuxerFrames::reset(Locker &locker)
{
    Q_D(QAVMuxerFrames);
    stop(locker);
    d->encStreams.clear();
    QAVMuxer::reset(locker);
}

int QAVMuxerFrames::flushFrames(Locker &locker)
{
    Q_D(QAVMuxerFrames);
    for (auto &s : d->encStreams) {
        // no flushing for subtitles
        bool isSub = s.codec()->avctx()->codec_type == AVMEDIA_TYPE_SUBTITLE;
        if (isSub)
            continue;
        int ret = write(QAVFrame(), s.index(), locker);
        if (ret < 0 && ret != AVERROR_EOF) {
            qWarning() << d->filename << ": Could not flush:" << err2str(ret);
            return ret;
        }
    }
    return 0;
}

class QAVMuxerSubtitleFramesPrivate : public QObject
{
    Q_DECLARE_PUBLIC(QAVMuxerSubtitleFrames)
public:
    QAVMuxerSubtitleFramesPrivate(QAVMuxerSubtitleFrames *q)
        : q_ptr(q)
    {
    }

    QAVMuxerSubtitleFrames *q_ptr = nullptr;
    QAVStream outputStream;
    QSharedPointer<QAVFormatContext> ctx;
};

QAVMuxerSubtitleFrames::QAVMuxerSubtitleFrames()
    : d_ptr(new QAVMuxerSubtitleFramesPrivate(this))
{
}

QAVMuxerSubtitleFrames::~QAVMuxerSubtitleFrames()
{
    unload();
}

int QAVMuxerSubtitleFrames::load(const QAVStream &stream)
{
    Q_D(QAVMuxerSubtitleFrames);
    auto encoder = avcodec_find_encoder(AV_CODEC_ID_SUBRIP);
    if (!encoder) {
        qWarning() << "Encoder not found";
        return AVERROR_INVALIDDATA;
    }

    d_ptr->ctx = QAVFormatContext::alloc();
    auto in_stream = stream.stream();
    auto dec_ctx = stream.codec()->avctx();
    auto out_stream = avformat_new_stream(d->ctx->ctx(), NULL);

    QSharedPointer<QAVCodec> codec(new QAVSubtitleCodec(encoder));
    auto enc_ctx = codec->avctx();
    enc_ctx->time_base = AV_TIME_BASE_Q;
    enc_ctx->height = dec_ctx->height;
    enc_ctx->width = dec_ctx->width;
    if (dec_ctx->subtitle_header) {
        // ASS code assumes this buffer is null terminated so add extra byte.
        enc_ctx->subtitle_header = (uint8_t*) av_mallocz(dec_ctx->subtitle_header_size + 1);
        if (!enc_ctx->subtitle_header)
            return {};
        memcpy(enc_ctx->subtitle_header, dec_ctx->subtitle_header,
               dec_ctx->subtitle_header_size);
        enc_ctx->subtitle_header_size = dec_ctx->subtitle_header_size;
    }
    if (!codec->open(out_stream)) {
        qWarning() << "Cannot open encoder: " << encoder->name;
        return AVERROR_INVALIDDATA;
    }
    auto ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    if (ret < 0) {
        qWarning() << "Copying parameters for stream failed:" << err2str(ret);
        return ret;
    }
    out_stream->time_base = in_stream->time_base;
    d->outputStream = {0, d->ctx, codec};
    return 0;
}

void QAVMuxerSubtitleFrames::unload()
{
    Q_D(QAVMuxerSubtitleFrames);
    d->ctx = nullptr;
    d->outputStream = {};
}

int QAVMuxerSubtitleFrames::parseText(const QAVSubtitleFrame &f, QString &out)
{
    Q_D(QAVMuxerSubtitleFrames);
    if (!d->outputStream || !f.stream())
        return 0;
    QAVSubtitleFrame frame = f;
    frame.setStream(d->outputStream);
    int sent = frame.send();
    if (sent < 0 && sent != AVERROR(EAGAIN))
        return sent;
    QAVPacket pkt;
    pkt.setStream(d->outputStream);
    int received = pkt.receive();
    if (received < 0)
        return received;
    out = QString::fromUtf8(reinterpret_cast<const char*>(pkt.packet()->data));
    return 0;
}

#if defined(QT_AVPLAYER_LIBASS)
class QAVASSRendererPrivate : public QObject
{
    Q_DECLARE_PUBLIC(QAVASSRenderer)
public:
    QAVASSRendererPrivate(QAVASSRenderer *q)
        : q_ptr(q)
    {
    }

    QAVASSRenderer *q_ptr = nullptr;
    ASS_Library *library = nullptr;
    ASS_Renderer *renderer = nullptr;
    ASS_Track *track = nullptr;
    QImage image;
};

QAVASSRenderer::QAVASSRenderer()
    : d_ptr(new QAVASSRendererPrivate(this))
{
}

QAVASSRenderer::~QAVASSRenderer()
{
    unload();
}

int QAVASSRenderer::load(const QAVStream &stream)
{
    Q_D(QAVASSRenderer);
    unload();
    d->library = ass_library_init();
    if (!d->library) {
        qWarning() << "Could not initialize libass";
        return AVERROR(EINVAL);
    }
    d->renderer = ass_renderer_init(d->library);
    if (!d->renderer) {
        qWarning() << "Could not initialize libass renderer";
        return AVERROR(EINVAL);
    }
    ass_set_fonts(d->renderer, NULL, NULL, 1, NULL, 1);
    d->track = ass_new_track(d->library);
    if (!d->track) {
        qWarning() << "Could not create a libass track";
        return AVERROR(EINVAL);
    }
    auto dec_ctx = stream.codec()->avctx();
    if (dec_ctx->subtitle_header) {
        ass_process_codec_private(d->track,
                                  (char*)dec_ctx->subtitle_header,
                                  dec_ctx->subtitle_header_size);
    }
    return 0;
}

void QAVASSRenderer::unload()
{
    Q_D(QAVASSRenderer);
    if (d->track)
        ass_free_track(d->track);
    if (d->renderer)
        ass_renderer_done(d->renderer);
    if (d->library)
        ass_library_done(d->library);
    d->track = nullptr;
    d->renderer = nullptr;
    d->library = nullptr;
}

QImage QAVASSRenderer::toImage(const QAVSubtitleFrame &frame, int width, int height)
{
    Q_D(QAVASSRenderer);
    if (!d->renderer)
        return d->image;
    auto sub = frame.subtitle();
    const int64_t start_time = av_rescale_q(sub->pts, AV_TIME_BASE_Q, av_make_q(1, 1000));
    const int64_t duration = sub->end_display_time;
    ass_set_frame_size(d->renderer, width, height);
    for (size_t i = 0; i < sub->num_rects; ++i) {
        char *ass_line = sub->rects[i]->ass;
        if (!ass_line)
            break;
        ass_process_chunk(d->track, ass_line, strlen(ass_line), start_time, duration);
    }
    int detect_change = 0;
    ASS_Image *image = ass_render_frame(d->renderer, d->track, start_time, &detect_change);
    if (detect_change == 0)
        return d->image;
    d->image = {width, height, QImage::Format_RGBA8888_Premultiplied};
    d->image.fill(Qt::transparent);

    for (ASS_Image *i = image; i; i = i->next) {
        int r = (i->color >> 24) & 0xFF;
        int g = (i->color >> 16) & 0xFF;
        int b = (i->color >> 8) & 0xFF;
        int a = 255 - (i->color & 0xFF); // libass uses inverted alpha

        for (int y = 0; y < i->h; ++y) {
            for (int x = 0; x < i->w; ++x) {
                int src_alpha = i->bitmap[y * i->stride + x];
                if (src_alpha == 0) continue;
                int dst_x = i->dst_x + x;
                int dst_y = i->dst_y + y;
                if (dst_x < 0 || dst_y < 0 || dst_x >= width || dst_y >= height)
                    continue;

                QColor dst = d->image.pixelColor(dst_x, dst_y);
                float alpha = (src_alpha / 255.0f) * (a / 255.0f);
                int out_r = r * alpha + dst.red() * (1 - alpha);
                int out_g = g * alpha + dst.green() * (1 - alpha);
                int out_b = b * alpha + dst.blue() * (1 - alpha);
                d->image.setPixelColor(dst_x, dst_y, QColor(out_r, out_g, out_b, 255));
            }
        }
    }

    return d->image;
}

void QAVASSRenderer::flush()
{
    Q_D(QAVASSRenderer);
    if (d->track)
        ass_flush_events(d->track);
    d->image = {};
}

#endif  // #if defined(QT_AVPLAYER_LIBASS)

QT_END_NAMESPACE
