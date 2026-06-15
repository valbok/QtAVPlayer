/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavmuxerpackets.h"
#include "qavmuxer_p_p.h"
#include "qavpacket.h"
#include "qavformatcontext_p.h"

#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QAVMuxerPackets::~QAVMuxerPackets()
{
    unload();
}

int QAVMuxerPackets::load(const QList<QAVStream> &streams, const QString &filename)
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    reset(locker);
    int ret = allocFormatContext(filename, locker);
    if (ret < 0)
        return ret;
    ret = initStreams(streams, locker);
    if (ret < 0)
        return ret;
    return writeHeader(locker);
}

int QAVMuxerPackets::initStreams(const QList<QAVStream> &streams, Locker &locker)
{
    Q_D(QAVMuxer);
    int ret = 0;
    for (int i = 0; i < streams.size(); ++i) {
        auto &stream = streams[i];
        ret = newOutputStream(stream, locker);
        if (ret < 0)
            return ret;
        ret = initStream(stream, i, d->ctx->ctx()->streams[i], locker);
        if (ret < 0)
            return ret;
    }
    return 0;
}

int QAVMuxerPackets::initStream(const QAVStream &stream, int, AVStream *out_stream, Locker &)
{
    auto in_stream = stream.stream();
    int ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    if (ret < 0) {
        qWarning() << "Failed to copy codec parameters:" << QAVMuxerPrivate::err2str(ret);
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
        return AVERROR(EINVAL);
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
            qWarning() << d->filename << ": Could not flush:" << QAVMuxerPrivate::err2str(ret);
            return ret;
        }
    }
    return 0;
}

QT_END_NAMESPACE
