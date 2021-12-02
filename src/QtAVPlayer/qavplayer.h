/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVPLAYER_H
#define QAVPLAYER_H

#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudioframe.h>
#include <QtAVPlayer/qtavplayerglobal.h>
#include <QString>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE

class QIODevice;
class QAVPlayerPrivate;
class Q_AVPLAYER_EXPORT QAVPlayer : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_ENUMS(MediaStatus)
    Q_ENUMS(Error)

public:
    enum State
    {
        StoppedState,
        PlayingState,
        PausedState
    };

    enum MediaStatus
    {
        NoMedia,
        LoadedMedia,
        EndOfMedia,
        InvalidMedia
    };

    enum Error
    {
        NoError,
        ResourceError,
        FilterError
    };

    QAVPlayer(QObject *parent = nullptr);
    ~QAVPlayer();

    void setSource(const QString &url, QIODevice *dev = nullptr);
    QString source() const;

    bool hasVideo() const;
    bool hasAudio() const;

    int videoStreamsCount() const;
    int videoStream() const;
    void setVideoStream(int stream);

    int audioStreamsCount() const;
    int audioStream() const;
    void setAudioStream(int stream);

    int subtitleStreamsCount() const;

    State state() const;
    MediaStatus mediaStatus() const;
    qint64 duration() const;
    qint64 position() const;
    qreal speed() const;
    double videoFrameRate() const;

    void setFilter(const QString &desc);
    QString filter() const;

    bool isSeekable() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void seek(qint64 position);
    void setSpeed(qreal rate);
    void stepForward();
    void stepBackward();

Q_SIGNALS:
    void sourceChanged(const QString &url);
    void stateChanged(QAVPlayer::State newState);
    void mediaStatusChanged(QAVPlayer::MediaStatus status);
    void errorOccurred(QAVPlayer::Error, const QString &str);
    void durationChanged(qint64 duration);
    void seekableChanged(bool seekable);
    void speedChanged(qreal rate);
    void videoFrameRateChanged(double rate);
    void videoStreamChanged(int stream);
    void audioStreamChanged(int stream);
    void played(qint64 pos);
    void paused(qint64 pos);
    void stopped(qint64 pos);
    void stepped(qint64 pos);
    void seeked(qint64 pos);
    void filterChanged(const QString &desc);

    void videoFrame(const QAVVideoFrame &frame);
    void audioFrame(const QAVAudioFrame &frame);

protected:
    QScopedPointer<QAVPlayerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVPlayer)
    Q_DECLARE_PRIVATE(QAVPlayer)
};

#ifndef QT_NO_DEBUG_STREAM
Q_AVPLAYER_EXPORT QDebug operator<<(QDebug, QAVPlayer::State);
Q_AVPLAYER_EXPORT QDebug operator<<(QDebug, QAVPlayer::MediaStatus);
Q_AVPLAYER_EXPORT QDebug operator<<(QDebug, QAVPlayer::Error);
#endif

Q_DECLARE_METATYPE(QAVPlayer::State)
Q_DECLARE_METATYPE(QAVPlayer::MediaStatus)
Q_DECLARE_METATYPE(QAVPlayer::Error)

QT_END_NAMESPACE

#endif
