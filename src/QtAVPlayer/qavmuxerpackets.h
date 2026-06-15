/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVMUXERPACKETS_H
#define QAVMUXERPACKETS_H

#include <QtAVPlayer/qavmuxer.h>

QT_BEGIN_NAMESPACE

class QAVPacket;

/**
 * Remuxes the same packets from demuxer.
 * No re-encoding.
 */
class QAVMuxerPackets : public QAVMuxer
{
public:
    QAVMuxerPackets() = default;
    ~QAVMuxerPackets() override;

    // Loads the encoder based on parsed streams, format is negotiated from filename
    int load(const QList<QAVStream> &streams, const QString &filename);

    // Writes the packet directly to the encoder
    int write(const QAVPacket &packet);

private:
    int initStreams(const QList<QAVStream> &streams, Locker &);
    int initStream(const QAVStream &stream, int index, AVStream *out_stream, Locker &);
    int flushFrames(Locker &) override;
    // Need to make a copy of packet
    int write(QAVPacket packet, int streamIndex, Locker &);
};

QT_END_NAMESPACE

#endif
