/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavsubtitletextparser.h"
#include "qavmuxer_p_p.h"
#include "qavcodec_p.h"
#include "qavsubtitlecodec_p.h"
#include "qavformatcontext_p.h"

#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/pixdesc.h>
}

QT_BEGIN_NAMESPACE

class QAVSubtitleTextParserPrivate
{
    Q_DECLARE_PUBLIC(QAVSubtitleTextParser)
public:
    QAVSubtitleTextParserPrivate(QAVSubtitleTextParser *q)
        : q_ptr(q)
    {
    }

    QAVSubtitleTextParser *q_ptr = nullptr;
    QAVStream outputStream;
    QSharedPointer<QAVFormatContext> ctx;
};

QAVSubtitleTextParser::QAVSubtitleTextParser()
    : d_ptr(new QAVSubtitleTextParserPrivate(this))
{
}

QAVSubtitleTextParser::~QAVSubtitleTextParser()
{
    unload();
}

int QAVSubtitleTextParser::load(const QAVStream &stream)
{
    Q_D(QAVSubtitleTextParser);
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
        qWarning() << "Copying parameters for stream failed:" << QAVMuxerPrivate::err2str(ret);
        return ret;
    }
    out_stream->time_base = in_stream->time_base;
    d->outputStream = {0, d->ctx, codec};
    return 0;
}

void QAVSubtitleTextParser::unload()
{
    Q_D(QAVSubtitleTextParser);
    d->ctx.clear();
    d->outputStream = {};
}

int QAVSubtitleTextParser::parseText(const QAVSubtitleFrame &f, QString &out)
{
    Q_D(QAVSubtitleTextParser);
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

QT_END_NAMESPACE
