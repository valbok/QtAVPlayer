/***************************************************************
 * Copyright (C) 2020, 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVPACKET_H
#define QAVPACKET_H

#include "qavframe.h"
#include "qavstream.h"
#include <memory>

QT_BEGIN_NAMESPACE

struct AVPacket;
class QAVPacketPrivate;
class QAVPacket
{
public:
    QAVPacket();
    ~QAVPacket();
    QAVPacket(const QAVPacket &other);
    QAVPacket &operator=(const QAVPacket &other);
    operator bool() const;

    AVPacket *packet() const;
    double duration() const;
    double pts() const;

    QAVStream stream() const;
    void setStream(const QAVStream &stream);

    // Receives a data from the codec from the stream
    int receive();

    // Sends the packet to the codec
    int send() const;

protected:
    std::unique_ptr<QAVPacketPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QAVPacket)
};

QT_END_NAMESPACE

#endif
