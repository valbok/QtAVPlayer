/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavmuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavsubtitlecodec_p.h"
#include "qavvideoframe.h"

#include <QObject>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>
#include <QDebug>

extern "C" {
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

    QAVMuxer *q_ptr = nullptr;
    AVFormatContext *ctx = nullptr;
    QList<QAVStream> streams;
    QString filename;
    bool loaded = false;

    std::unique_ptr<QThread> workerThread;
    QList<QAVFrame> frames;
    mutable QMutex mutex;
    QWaitCondition cond;
    bool quit = false;

    void doWork();
};

QAVMuxer::QAVMuxer()
    : d_ptr(new QAVMuxerPrivate(this))
{
}

QAVMuxer::~QAVMuxer()
{
    unload();
}

void QAVMuxer::reset()
{
    Q_D(QAVMuxer);
    close();
    if (d->ctx && !(d->ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&d->ctx->pb);
    d->streams.clear();
    avformat_free_context(d->ctx);
    d->ctx = nullptr;
}

void QAVMuxer::close()
{
    Q_D(QAVMuxer);
    if (d->ctx && d->loaded) {
        flush();
        av_write_trailer(d->ctx);
    }
    d->loaded = false;
}

int QAVMuxer::load(const AVFormatContext *ictx, const QList<QAVStream> &streams, const QString &filename)
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    reset();
    int ret = avformat_alloc_output_context2(&d->ctx, nullptr, nullptr, filename.toUtf8().constData());
    if (ret < 0) {
        qWarning() << filename << ": Could not create output context:" << err2str(ret);
        return ret;
    }
    d->filename = filename;
    Q_ASSERT(streams.size() == int(ictx->nb_streams));
    for (unsigned i = 0; i < ictx->nb_streams; ++i) {
        auto stream = streams[i];
        auto in_stream = stream.stream();
        auto dec_ctx = stream.codec()->avctx();
        auto out_stream = avformat_new_stream(d->ctx, NULL);
        if (!out_stream) {
            qWarning() << "Failed allocating output stream";
            return AVERROR_UNKNOWN;
        }

        const auto pix_fmt_desc = av_pix_fmt_desc_get(dec_ctx->pix_fmt);
        qDebug() << "[" << filename << "][" << i << "][" <<
            av_get_media_type_string(dec_ctx->codec_type) << "]: Using" <<
            stream.codec()->codec()->name << ", codec_id" << dec_ctx->codec_id <<
            ", pix_fmt:" << dec_ctx->pix_fmt << (pix_fmt_desc ? pix_fmt_desc->name : "");
        // Transcoding to same codec
        auto encoder = avcodec_find_encoder(dec_ctx->codec_id);
        if (!encoder) {
            qWarning() << "Encoder not found:" << stream.codec()->codec()->name;
            return AVERROR_INVALIDDATA;
        }
        AVCodecContext *enc_ctx = nullptr;
        QSharedPointer<QAVCodec> codec;

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
            if (d->ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            if (!codec->open(out_stream)) {
                qWarning() << i << ": Cannot open encoder: " << encoder->name;
                return AVERROR_UNKNOWN;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                qWarning() << "Failed to copy encoder parameters to output stream:" << err2str(ret);
                return ret;
            }
            out_stream->time_base = enc_ctx->time_base;
            d->streams.push_back({ int(i), d->ctx, codec });
            break;
        }
        case AVMEDIA_TYPE_AUDIO: {
            codec.reset(new QAVAudioCodec(encoder));
            enc_ctx = codec->avctx();
            enc_ctx->sample_rate = dec_ctx->sample_rate;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
            ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
            if (ret < 0) {
                qWarning() << "Failed av_channel_layout_copy:" << err2str(ret);
                return ret;
            }
#endif
            enc_ctx->sample_fmt = dec_ctx->sample_fmt;
            enc_ctx->time_base = in_stream->time_base;
            if (d->ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            if (!codec->open(out_stream)) {
                qWarning() << i << ": Cannot open encoder: " << encoder->name;
                return AVERROR_UNKNOWN;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                qWarning() << "Failed to copy encoder parameters to output stream:" << err2str(ret);
                return ret;
            }
            out_stream->time_base = enc_ctx->time_base;
            d->streams.push_back({ int(i), d->ctx, codec });
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
                qWarning() << i << ": Cannot open encoder: " << encoder->name;
                return AVERROR_UNKNOWN;
            }
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                qWarning() << "Copying parameters for stream failed:" << err2str(ret);
                return ret;
            }
            out_stream->time_base = in_stream->time_base;
            d->streams.push_back({ int(i), d->ctx, codec });
            break;
        }
        default:
            qWarning() << "Unexpected stream:" << dec_ctx->codec_type;
            return AVERROR_INVALIDDATA;
        }
    }
    av_dump_format(d->ctx, 0, filename.toUtf8().constData(), 1);

    if (!(d->ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&d->ctx->pb, filename.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            qWarning() <<" Could not open output file:" << filename << ":"<< err2str(ret);
            return ret;
        }
    }

    // Init muxer, write output file header
    ret = avformat_write_header(d->ctx, NULL);
    if (ret < 0) {
        qWarning() << "Error occurred when opening output file:" << err2str(ret);
        return ret;
    }

    d->quit = false;
    d->workerThread.reset(new QThread);
    QObject::connect(d->workerThread.get(), &QThread::started, d, &QAVMuxerPrivate::doWork, Qt::DirectConnection);
    d->workerThread->start();
    d->loaded = true;
    return 0;
}

void QAVMuxer::unload()
{
    Q_D(QAVMuxer);
    stop();
    auto loaded = false;
    {
        QMutexLocker locker(&d->mutex);
        loaded = d->loaded;
    }
    if (loaded) {
        d->workerThread->quit();
        d->workerThread->wait();
    }
    reset();
}

void QAVMuxer::enqueue(const QAVFrame &frame)
{
    Q_D(QAVMuxer);
    {
        QMutexLocker locker(&d->mutex);
        if (!d->loaded)
            return;
        d->frames.push_back(frame);
    }
    d->cond.wakeAll();
}

size_t QAVMuxer::size() const
{
    Q_D(const QAVMuxer);
    QMutexLocker locker(&d->mutex);
    return d->frames.size();
}

void QAVMuxerPrivate::doWork()
{
    QMutexLocker locker(&mutex);
    while (!quit) {
        if (frames.isEmpty()) {
            cond.wait(&mutex);
            if (quit || frames.isEmpty())
                break;
        }
        auto frame = frames.takeFirst();
        q_ptr->write(frame, frame.stream().index());
    }
}

int QAVMuxer::write(const QAVFrame &frame)
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    if (!d->loaded)
        return 0;
    return write(frame, frame.stream().index());
}

int QAVMuxer::write(const QAVSubtitleFrame &frame)
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    if (!d->loaded)
        return 0;
    return write(frame, frame.stream().index());
}

int QAVMuxer::write(QAVFrame frame, int streamIndex)
{
    Q_D(QAVMuxer);
    Q_ASSERT(streamIndex < d->streams.size());
    auto &encStream = d->streams[streamIndex];
    auto enc_ctx = encStream.codec()->avctx();
    auto stream = encStream.stream();

    if (enc_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
        // If the frame is in different format than the encoder
        // TODO: avoid converting
        if (frame && frame.frame()->format != enc_ctx->pix_fmt) {
            QAVVideoFrame videoFrame = frame;
            frame = videoFrame.convertTo(enc_ctx->pix_fmt);
        }
        frame.frame()->pict_type = AV_PICTURE_TYPE_NONE;
    }

    // Set encoder stream to frame to send to
    frame.setStream(encStream);
    int sent = 0;
    do {
        sent = frame.send();
        if (sent < 0 && sent != AVERROR(EAGAIN)){
            return sent;
        }

        int wrote = 0;
        while (wrote >= 0) {
            QAVPacket pkt;
            pkt.setStream(encStream);
            int received = pkt.receive();
            if (received < 0){
                break;
            }
            auto enc_pkt = pkt.packet();
            enc_pkt->stream_index = streamIndex;
            av_packet_rescale_ts(enc_pkt, enc_ctx->time_base, stream->time_base);
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
            enc_pkt->time_base = stream->time_base;
#endif
            wrote = av_interleaved_write_frame(d->ctx, enc_pkt);
        }
    } while (sent == AVERROR(EAGAIN));
    return 0;
}

int QAVMuxer::write(QAVSubtitleFrame frame, int streamIndex)
{
    Q_D(QAVMuxer);
    Q_ASSERT(streamIndex < d->streams.size());
    auto &encStream = d->streams[streamIndex];
    auto enc_ctx = encStream.codec()->avctx();
    auto stream = encStream.stream();

    // Set encoder stream to frame to send to
    frame.setStream(encStream);
    int sent = frame.send();
    if (sent < 0 && sent != AVERROR(EAGAIN)){
        return sent;
    }
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
    int wrote = av_interleaved_write_frame(d->ctx, enc_pkt);
    return wrote;
}

int QAVMuxer::flush()
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    for (int i = 0; i < d->streams.size(); ++i) {
        // no flushing for subtitles
        bool isSub = d->streams[i].codec()->avctx()->codec_type == AVMEDIA_TYPE_SUBTITLE;
        if (isSub)
            continue;
        int ret = write(QAVFrame(), i);
        if (ret < 0 && ret != AVERROR_EOF) {
            qWarning() << d->filename << ": Could not flush:" << err2str(ret);
            return ret;
        }
    }
    return 0;
}

void QAVMuxer::stop()
{
    Q_D(QAVMuxer);
    {
        QMutexLocker locker(&d->mutex);
        d->quit = true;
    }
    d->cond.wakeAll();
}

QT_END_NAMESPACE
