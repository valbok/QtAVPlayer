/***************************************************************
 * Copyright (C) 2020, 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVCODEC_P_H
#define QAVCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qavpacket.h"
#include "qavframe.h"
#include <QtAVPlayer/qtavplayerglobal.h>
#include <memory>

extern "C" {
#include <libavutil/dict.h>
}

QT_BEGIN_NAMESPACE

struct AVCodec;
struct AVCodecContext;
struct AVStream;
class QAVCodecPrivate;
class QAVCodec
{
public:
    virtual ~QAVCodec();

    bool open(AVStream *stream, AVDictionary** opts = NULL);
    AVCodecContext *avctx() const;
    void setCodec(const AVCodec *c);
    const AVCodec *codec() const;

    void flushBuffers();

    // Sends a packet
    virtual int write(const QAVPacket &pkt) = 0;
    // Sends a frame
    virtual int write(const QAVStreamFrame &frame) = 0;
    // Receives a frame
    // NOTE: There could be multiple frames
    virtual int read(QAVStreamFrame &frame) = 0;
    // Receives a packet
    virtual int read(QAVPacket &pkt) = 0;

protected:
    QAVCodec();
    QAVCodec(QAVCodecPrivate &d, const AVCodec *codec = nullptr);

    std::unique_ptr<QAVCodecPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVCodec)
private:
    Q_DISABLE_COPY(QAVCodec)
};

QT_END_NAMESPACE

#endif
