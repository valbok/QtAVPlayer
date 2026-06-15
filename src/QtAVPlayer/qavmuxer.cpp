/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavmuxer.h"
#include "qavmuxer_p_p.h"
#include "qavsubtitlecodec_p.h"
#include "qavformatcontext_p.h"

#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

char *QAVMuxerPrivate::err2str(int errnum)
{
    thread_local char str[AV_ERROR_MAX_STRING_SIZE];
    memset(str, 0, sizeof(str));
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

int QAVMuxerPrivate::outputStreamIndex(const QAVStream &stream, QAVMuxer::Locker &) const
{
    if (!stream)
        return -1;
    auto it = outputStreams.find(stream.stream());
    if (it == outputStreams.end())
        return -1;
    return *it;
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

int QAVMuxer::allocFormatContext(const QString &filename, Locker &)
{
    Q_D(QAVMuxer);
    d->ctx = QAVFormatContext::alloc(filename);
    if (!d->ctx)
        return AVERROR(EINVAL);
    d->filename = filename;
    int ret = 0;
    if (!(d->ctx->ctx()->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&d->ctx->ctx()->pb, filename.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0)
            qWarning() << "Could not open output file:" << d->filename << ":"<< QAVMuxerPrivate::err2str(ret);
    }
    return ret;
}

int QAVMuxer::writeHeader(Locker &)
{
    Q_D(QAVMuxer);
    av_dump_format(d->ctx->ctx(), 0, d->filename.toUtf8().constData(), 1);
    // Init muxer, write output file header
    int ret = avformat_write_header(d->ctx->ctx(), nullptr);
    if (ret < 0) {
        qWarning() << d->filename << ": Failed avformat_write_header:" << QAVMuxerPrivate::err2str(ret);
        return ret;
    }
    d->loaded = true;
    return 0;
}

int QAVMuxer::newOutputStream(const QAVStream &stream, Locker &)
{
    Q_D(QAVMuxer);
    auto codec = stream.codec();
    if (!codec)
        return AVERROR(EINVAL);
    auto out_stream = avformat_new_stream(d->ctx->ctx(), NULL);
    if (!out_stream) {
        qWarning() << "Failed allocating output stream";
        return AVERROR_UNKNOWN;
    }
    auto dec_ctx = codec->avctx();
    const auto pix_fmt_desc = av_pix_fmt_desc_get(dec_ctx->pix_fmt);
    qDebug() << "[" << d->filename << "][" << stream.index() << "][" <<
        av_get_media_type_string(dec_ctx->codec_type) << "]: Using" <<
        stream.codec()->codec()->name << ", codec_id" << dec_ctx->codec_id <<
        ", pix_fmt:" << dec_ctx->pix_fmt << (pix_fmt_desc ? pix_fmt_desc->name : "");
    auto i = d->ctx->ctx()->nb_streams - 1;
    d->outputStreams[stream.stream()] = i;
    return 0;
}

int QAVMuxer::flush()
{
    Q_D(QAVMuxer);
    QMutexLocker locker(&d->mutex);
    if (!d->loaded)
        return 0;
    return flushFrames(locker);
}

QT_END_NAMESPACE
