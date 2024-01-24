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
#include "qavstream.h"
#include "qavframe.h"
#include "qavsubtitleframe.h"
#include <QMap>
#include <memory>

QT_BEGIN_NAMESPACE

extern "C" {
#include <libavutil/avutil.h>
}

class QAVDemuxerPrivate;
class QAVVideoCodec;
class QAVAudioCodec;
class QAVIODevice;
struct AVStream;
struct AVCodecContext;
struct AVFormatContext;
class QAVDemuxer
{
public:
    QAVDemuxer();
    ~QAVDemuxer();

    void abort(bool stop = true);
    int load(const QString &url, QAVIODevice *dev = nullptr);
    void unload();

    AVMediaType currentCodecType(int index) const;

    QList<QAVStream> availableVideoStreams() const;
    QList<QAVStream> currentVideoStreams() const;
    bool setVideoStreams(const QList<QAVStream> &streams);

    QList<QAVStream> availableAudioStreams() const;
    QList<QAVStream> currentAudioStreams() const;
    bool setAudioStreams(const QList<QAVStream> &streams);

    QList<QAVStream> availableSubtitleStreams() const;
    QList<QAVStream> currentSubtitleStreams() const;
    bool setSubtitleStreams(const QList<QAVStream> &streams);

    QAVPacket read();

    void decode(const QAVPacket &pkt, QList<QAVFrame> &frames) const;
    void decode(const QAVPacket &pkt, QList<QAVSubtitleFrame> &frames) const;
    void flushCodecBuffers();

    double duration() const;
    bool seekable() const;
    int seek(double sec);
    bool eof() const;
    double videoFrameRate() const;

    QMap<QString, QString> metadata() const;

    QString bitstreamFilter() const;
    int applyBitstreamFilter(const QString &bsfs);

    QString inputFormat() const;
    void setInputFormat(const QString &format);

    QString inputVideoCodec() const;
    void setInputVideoCodec(const QString &codec);

    QMap<QString, QString> inputOptions() const;
    void setInputOptions(const QMap<QString, QString> &opts);

    void onFrameSent(const QAVStreamFrame &frame);
    QAVStream::Progress progress(const QAVStream &s) const;

    bool isMasterStream(const QAVStream &stream) const;

    static QStringList supportedFormats();
    static QStringList supportedVideoCodecs();
    static QStringList supportedProtocols();
    static QStringList supportedBitstreamFilters();

protected:
    std::unique_ptr<QAVDemuxerPrivate> d_ptr;

private:
    int resetCodecs();

    Q_DISABLE_COPY(QAVDemuxer)
    Q_DECLARE_PRIVATE(QAVDemuxer)
};

QT_END_NAMESPACE

#endif
