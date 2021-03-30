/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVDEMUXER_H
#define QAVDEMUXER_H

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

#include "qavpacket_p.h"
#include "qtavplayerglobal_p.h"
#include <QObject>
#include <QUrl>

QT_BEGIN_NAMESPACE

class QAVDemuxerPrivate;
class QAVVideoCodec;
class QAVAudioCodec;
class Q_AVPLAYER_EXPORT QAVDemuxer : public QObject
{
public:
    QAVDemuxer(QObject *parent = nullptr);
    ~QAVDemuxer();

    void abort(bool stop = true);
    int load(const QUrl &url);
    void unload();

    int videoStream() const;
    void setVideoCodec(QAVVideoCodec *);
    QAVVideoCodec *videoCodec() const;
    int audioStream() const;
    QAVAudioCodec *audioCodec() const;
    int subtitleStream() const;

    QAVPacket read();

    double duration() const;
    bool seekable() const;
    int seek(double sec);
    bool eof() const;
    double frameRate() const;

protected:
    QScopedPointer<QAVDemuxerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVDemuxer)
    Q_DECLARE_PRIVATE(QAVDemuxer)
};

QT_END_NAMESPACE

#endif
