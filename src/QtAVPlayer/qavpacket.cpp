/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavpacket_p.h"
#include "qavcodec_p.h"
#include "qavstream.h"
#include <QSharedPointer>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}

QT_BEGIN_NAMESPACE

class QAVPacketPrivate
{
public:
    AVPacket *pkt = nullptr;
    QAVStream stream;
};

QAVPacket::QAVPacket()
    : d_ptr(new QAVPacketPrivate)
{
    d_ptr->pkt = av_packet_alloc();
    d_ptr->pkt->size = 0;
    d_ptr->pkt->stream_index = -1;
    d_ptr->pkt->pts = AV_NOPTS_VALUE;
}

QAVPacket::QAVPacket(const QAVPacket &other)
    : QAVPacket()
{
    *this = other;
}

QAVPacket &QAVPacket::operator=(const QAVPacket &other)
{
    av_packet_unref(d_ptr->pkt);
    av_packet_ref(d_ptr->pkt, other.d_ptr->pkt);

    d_ptr->stream = other.d_ptr->stream;

    return *this;
}

QAVPacket::operator bool() const
{
    Q_D(const QAVPacket);
    return d->pkt->size;
}

QAVPacket::~QAVPacket()
{
    Q_D(QAVPacket);
    av_packet_free(&d->pkt);
}

AVPacket *QAVPacket::packet() const
{
    return d_func()->pkt;
}

double QAVPacket::duration() const
{
    Q_D(const QAVPacket);
    if (!d->stream)
        return 0.0;
    auto tb = d->stream.stream()->time_base;
    return tb.num && tb.den ? d->pkt->duration * av_q2d(tb) : 0.0;
}

double QAVPacket::pts() const
{
    Q_D(const QAVPacket);
    if (!d->stream)
        return 0.0;
    auto tb = d->stream.stream()->time_base;
    return tb.num && tb.den ? d->pkt->pts * av_q2d(tb) : 0.0;
}

QAVStream QAVPacket::stream() const
{
    Q_D(const QAVPacket);
    return d->stream;
}

void QAVPacket::setStream(const QAVStream &stream)
{
    Q_D(QAVPacket);
    d->stream = stream;
}

int QAVPacket::send() const
{
    Q_D(const QAVPacket);
    return d->stream ? d->stream.codec()->write(*this) : 0;
}

QT_END_NAMESPACE
