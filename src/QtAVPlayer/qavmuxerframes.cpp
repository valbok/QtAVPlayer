/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavmuxerframes.h"
#include "qavmuxer_p_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavsubtitlecodec_p.h"
#include "qavvideoframe.h"
#include "qavformatcontext_p.h"

#include <QThread>
#include <QWaitCondition>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

class QAVMuxerFramesPrivate : public QAVMuxerPrivate
{
    Q_DECLARE_PUBLIC(QAVMuxerFrames)
public:
    QAVMuxerFramesPrivate(QAVMuxerFrames *q)
        : QAVMuxerPrivate(q)
    {
    }

    QString outputVideoCodec;
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
        if (!loaded || frames.isEmpty()) {
            cond.wait(&mutex);
            if (!loaded)
                continue;
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

QAVMuxerFrames::EncoderStream::EncoderStream(const QAVStream &stream)
    : m_stream(stream)
{
}

const QAVStream &QAVMuxerFrames::EncoderStream::stream() const
{
    return m_stream;
}

void QAVMuxerFrames::EncoderStream::setSize(const QSize &size)
{
    m_size = size;
}

const QSize &QAVMuxerFrames::EncoderStream::size() const
{
    return m_size;
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

void QAVMuxerFrames::setOutputVideoCodec(const QString &name)
{
    Q_D(QAVMuxerFrames);
    QMutexLocker locker(&d->mutex);
    d->outputVideoCodec = name;
}

QString QAVMuxerFrames::outputVideoCodec() const
{
    Q_D(const QAVMuxerFrames);
    QMutexLocker locker(&d->mutex);
    return d->outputVideoCodec;
}

void QAVMuxerFrames::enqueue(const QAVFrame &frame)
{
    Q_D(QAVMuxerFrames);
    {
        QMutexLocker locker(&d->mutex);
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

int QAVMuxerFrames::load(const QList<QAVStream> &streams, const QString &filename)
{
    return load(QList<EncoderStream>(streams.begin(), streams.end()), filename);
}

int QAVMuxerFrames::load(const QList<EncoderStream> &streams, const QString &filename)
{
    Q_D(QAVMuxerFrames);
    QMutexLocker locker(&d->mutex);
    reset(locker);
    int ret = allocFormatContext(filename, locker);
    if (ret < 0)
        return ret;
    ret = initStreams(streams, locker);
    if (ret < 0)
        return ret;
    init(locker);
    return writeHeader(locker);
}

int QAVMuxerFrames::initStreams(const QList<EncoderStream> &streams, Locker &locker)
{
    Q_D(QAVMuxerFrames);
    int ret = 0;
    for (int i = 0; i < streams.size(); ++i) {
        auto &stream = streams[i];
        ret = newOutputStream(stream.stream(), locker);
        if (ret < 0)
            return ret;
        ret = initStream(stream, i, d->ctx->ctx()->streams[i], locker);
        if (ret < 0)
            return ret;
    }
    return 0;
}

void QAVMuxerFrames::init(Locker &)
{
    Q_D(QAVMuxerFrames);
    d->quit = false;
    d->workerThread.reset(new QThread);
    QObject::connect(d->workerThread.get(), &QThread::started, d, &QAVMuxerFramesPrivate::doWork, Qt::DirectConnection);
    d->workerThread->start();
}

int QAVMuxerFrames::initStream(const EncoderStream &encoderStream, int index, AVStream *out_stream, Locker &)
{
    Q_D(QAVMuxerFrames);
    const auto &stream = encoderStream.stream();
    auto in_stream = stream.stream();
    auto dec_ctx = stream.codec()->avctx();
    const AVCodec *encoder = nullptr;
    switch (in_stream->codecpar->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        if (!d->outputVideoCodec.isEmpty()) {
            qDebug() << "Loading: -vcodec" << d->outputVideoCodec;
            encoder = avcodec_find_encoder_by_name(d->outputVideoCodec.toUtf8().constData());
            if (!encoder) {
                qWarning() << "Encoder not found:" << d->outputVideoCodec;
                return AVERROR(EINVAL);
            }
            break;
        }
        [[fallthrough]];
    default:
        // Transcoding to same codec
        encoder = avcodec_find_encoder(dec_ctx->codec_id);
        if (!encoder) {
            qWarning() << "Encoder not found:" << dec_ctx->codec_id;
            return AVERROR(EINVAL);
        }
    }

    AVCodecContext *enc_ctx = nullptr;
    QSharedPointer<QAVCodec> codec;
    int ret = 0;
    switch (dec_ctx->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
        codec.reset(new QAVVideoCodec(encoder));
        enc_ctx = codec->avctx();
        if (!encoderStream.size().isEmpty()) {
            enc_ctx->height = encoderStream.size().height();
            enc_ctx->width = encoderStream.size().width();
        } else {
            enc_ctx->height = dec_ctx->height;
            enc_ctx->width = dec_ctx->width;
        }
        enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
        // pix_fmt might be not supported by the encoder, f.e. vdpau
        // so falling back to software pixel format
        if (d->outputVideoCodec.isEmpty() && dec_ctx->sw_pix_fmt != AV_PIX_FMT_NONE)
            enc_ctx->pix_fmt = dec_ctx->sw_pix_fmt;
        else
            enc_ctx->pix_fmt = dec_ctx->pix_fmt;
        enc_ctx->time_base = in_stream->time_base;
        if (!d->outputVideoCodec.isEmpty() && dec_ctx->hw_frames_ctx)
            enc_ctx->hw_frames_ctx = av_buffer_ref(dec_ctx->hw_frames_ctx);
        if (d->ctx->ctx()->oformat->flags & AVFMT_GLOBALHEADER)
            enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        if (!codec->open(out_stream)) {
            qWarning() << stream.index() << ": Cannot open encoder:" << encoder->name;
            return AVERROR_UNKNOWN;
        }
        ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
        if (ret < 0) {
            qWarning() << stream.index() << ": Failed to copy encoder parameters to output stream:" << QAVMuxerPrivate::err2str(ret);
            return ret;
        }
        out_stream->time_base = enc_ctx->time_base;
        d->encStreams[index] = { index, d->ctx, codec };
        break;
    case AVMEDIA_TYPE_AUDIO:
        codec.reset(new QAVAudioCodec(encoder));
        enc_ctx = codec->avctx();
        enc_ctx->sample_rate = dec_ctx->sample_rate;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(59, 0, 0)
        ret = av_channel_layout_copy(&enc_ctx->ch_layout, &dec_ctx->ch_layout);
        if (ret < 0) {
            qWarning() << stream.index() << ": Failed av_channel_layout_copy:" << QAVMuxerPrivate::err2str(ret);
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
            qWarning() << stream.index() << ": Failed to copy encoder parameters to output stream:" << QAVMuxerPrivate::err2str(ret);
            return ret;
        }
        out_stream->time_base = enc_ctx->time_base;
        d->encStreams[index] = { index, d->ctx, codec };
        break;
    case AVMEDIA_TYPE_SUBTITLE:
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
            qWarning() << stream.index() << ": Copying parameters for stream failed:" << QAVMuxerPrivate::err2str(ret);
            return ret;
        }
        out_stream->time_base = in_stream->time_base;
        d->encStreams[index] = { index, d->ctx, codec };
        break;
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
        return AVERROR(EINVAL);
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
        return AVERROR(EINVAL);
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
            qWarning() << d->filename << ": Could not flush:" << QAVMuxerPrivate::err2str(ret);
            return ret;
        }
    }
    return 0;
}

QT_END_NAMESPACE
