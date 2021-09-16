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
#include <QObject>
#include <QUrl>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE

class QAVDemuxerPrivate;
class QAVVideoCodec;
class QAVAudioCodec;
struct AVStream;
struct AVCodecContext;
struct AVFormatContext;
class Q_AVPLAYER_EXPORT QAVDemuxer : public QObject
{
public:
    QAVDemuxer(QObject *parent = nullptr);
    ~QAVDemuxer();

    void abort(bool stop = true);
    int load(const QUrl &url);
    void unload();

    QList<int> videoStreams() const;
    int currentVideoStreamIndex() const;
    QList<int> audioStreams() const;
    int currentAudioStreamIndex() const;
    QList<int> subtitleStreams() const;
    int currentSubtitleStreamIndex() const;

    int videoStreamIndex() const;
    void setVideoStreamIndex(int stream);
    int audioStreamIndex() const;
    void setAudioStreamIndex(int stream);
    int subtitleStreamIndex() const;
    void setSubtitleStreamIndex(int stream);

    QAVPacket read();

    double duration() const;
    bool seekable() const;
    int seek(double sec);
    bool eof() const;
    double videoFrameRate() const;

    AVStream *videoStream() const;
    AVStream *audioStream() const;
    QSharedPointer<QAVVideoCodec> videoCodec() const;
    QSharedPointer<QAVAudioCodec> audioCodec() const;
    AVRational frameRate() const;

protected:
    QScopedPointer<QAVDemuxerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVDemuxer)
    Q_DECLARE_PRIVATE(QAVDemuxer)
};

QT_END_NAMESPACE

#endif
