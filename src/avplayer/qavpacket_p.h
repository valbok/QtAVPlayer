/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVPACKET_H
#define QAVPACKET_H

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

#include "qavframe_p.h"
#include <QObject>

QT_BEGIN_NAMESPACE

struct AVPacket;
class QAVPacketPrivate;
class QAVCodec;
class Q_AVPLAYER_EXPORT QAVPacket : public QObject
{
public:
    QAVPacket(QObject *parent = nullptr);
    ~QAVPacket();
    QAVPacket(const QAVPacket &other);
    QAVPacket &operator=(const QAVPacket &other);
    operator bool() const;

    AVPacket *packet();
    double duration() const;
    int bytes() const;
    int streamIndex() const;

    void setCodec(const QAVCodec *codec);
    QAVFrame decode();

protected:
    QScopedPointer<QAVPacketPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QAVPacket)
};

QT_END_NAMESPACE

#endif
