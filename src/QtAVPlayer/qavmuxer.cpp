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

        switch (dec_ctx->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_AUDIO: {
            // Transcoding to same codec
            auto encoder = avcodec_find_encoder(dec_ctx->codec_id);
            if (!encoder) {
                qWarning() << "Necessary encoder not found" << stream.codec()->codec()->name;
                return AVERROR_INVALIDDATA;
            }
            // pix_fmt might be not supported by the encoder, so falling back to software pixel format
            if (dec_ctx->sw_pix_fmt != AV_PIX_FMT_NONE)
                dec_ctx->pix_fmt = dec_ctx->sw_pix_fmt;
            auto pix_fmt_desc = av_pix_fmt_desc_get(dec_ctx->pix_fmt);
            qDebug() << "[" << filename << "][" << av_get_media_type_string(dec_ctx->codec_type) << "]: Using" <<
                stream.codec()->codec()->name << ", pix_fmt:" << dec_ctx->pix_fmt << (pix_fmt_desc ? pix_fmt_desc->name : "");
            QSharedPointer<QAVCodec> codec;
            AVCodecContext *enc_ctx = nullptr;
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                codec.reset(new QAVVideoCodec(encoder));
                enc_ctx = codec->avctx();

                enc_ctx->height = dec_ctx->height;
                enc_ctx->width = dec_ctx->width;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 31, 0)
                const enum AVPixelFormat *pix_fmts = NULL;
                ret = avcodec_get_supported_config(dec_ctx, NULL,
                                                   AV_CODEC_CONFIG_PIX_FORMAT, 0,
                                                   (const void**)&pix_fmts, NULL);
                // Take first format from list of supported formats
                enc_ctx->pix_fmt = (ret >= 0 && pix_fmts) ? pix_fmts[0] : dec_ctx->pix_fmt;
#else
                enc_ctx->pix_fmt = dec_ctx->pix_fmt;
#endif
                enc_ctx->time_base = in_stream->time_base;
            } else {
                codec.reset(new QAVAudioCodec(encoder));
                enc_ctx = codec->avctx();

                enc_ctx->sample_rate = dec_ctx->sample_rate;
                ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
                if (ret < 0)
                    return ret;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 31, 0)
                const enum AVSampleFormat *sample_fmts = NULL;
                ret = avcodec_get_supported_config(dec_ctx, NULL,
                                                   AV_CODEC_CONFIG_SAMPLE_FORMAT, 0,
                                                   (const void**)&sample_fmts, NULL);
                // Take first format from list of supported formats
                enc_ctx->sample_fmt = (ret >= 0 && sample_fmts) ? sample_fmts[0] : dec_ctx->sample_fmt;
#else
                enc_ctx->sample_fmt = dec_ctx->sample_fmt;
#endif

                enc_ctx->time_base = in_stream->time_base;
            }
            if (d->ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            Q_ASSERT(codec);
            if (!codec->open(out_stream)) {
                qWarning() << "Cannot open" << encoder->name << "encoder for stream" << i;
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
            // If this stream must be remuxed
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                qWarning() << "Copying parameters for stream failed:" << err2str(ret);
                return ret;
            }
            QSharedPointer<QAVCodec> codec(new QAVSubtitleCodec);
            if (!codec->open(out_stream)) {
                qWarning() << "Cannot open SUBTITLE encoder for stream:" << i;
                return AVERROR_UNKNOWN;
            }
            out_stream->time_base = in_stream->time_base;
            d->streams.push_back({ int(i), d->ctx, codec });
            break;
        }
        default:
            qWarning() << "Elementary stream is of unknown type, cannot proceed";
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
        q_ptr->write(frame);
    }
}

int QAVMuxer::write(const QAVFrame &frame)
{
    return write(frame, frame.stream().index());
}

int QAVMuxer::write(QAVFrame frame, int streamIndex)
{
    Q_D(QAVMuxer);
    Q_ASSERT(d->ctx);
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
            enc_pkt->time_base = stream->time_base;
            wrote = av_interleaved_write_frame(d->ctx, enc_pkt);
        }
    } while (sent == AVERROR(EAGAIN));
    return 0;
}

int QAVMuxer::flush() {
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    for (int i = 0; i < d->streams.size(); ++i) {
        int ret = write({}, i);
        if (ret < 0)
            return ret;
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
