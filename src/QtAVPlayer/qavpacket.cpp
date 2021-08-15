/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavpacket_p.h"
#include "qavcodec_p.h"
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
    QSharedPointer<QAVCodec> codec;
    AVPacket *pkt = nullptr;
};

QAVPacket::QAVPacket(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVPacketPrivate)
{
    d_ptr->pkt = av_packet_alloc();
    d_ptr->pkt->size = 0;
    d_ptr->pkt->stream_index = -1;
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

    d_ptr->codec = other.d_ptr->codec;

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

AVPacket *QAVPacket::packet()
{
    return d_func()->pkt;
}

double QAVPacket::duration() const
{
    Q_D(const QAVPacket);
    if (!d->codec || !d->codec->stream())
        return 0;

    return d->pkt->duration * av_q2d(d->codec->stream()->time_base);
}

double QAVPacket::pts() const
{
    Q_D(const QAVPacket);
    if (!d->codec || !d->codec->stream())
        return 0;

    return d->pkt->pts * av_q2d(d->codec->stream()->time_base);
}

int QAVPacket::bytes() const
{
    return d_func()->pkt->size;
}

int QAVPacket::streamIndex() const
{
    return d_func()->pkt->stream_index;
}

void QAVPacket::setCodec(const QSharedPointer<QAVCodec> &codec)
{
    Q_D(QAVPacket);
    d->codec = codec;
}

QAVFrame QAVPacket::decode()
{
    Q_D(QAVPacket);
    if (!d->codec)
        return {};

    QAVFrame frame(d->codec);
    if (d->codec->decode(d->pkt, frame))
        return frame;
    return {};
}

QT_END_NAMESPACE
