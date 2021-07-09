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
#include <QUrl>
#include <QScopedPointer>
#include <functional>

QT_BEGIN_NAMESPACE

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
        LoadingMedia,
        SeekingMedia,
        LoadedMedia,
        EndOfMedia,
        InvalidMedia
    };

    enum Error
    {
        NoError,
        ResourceError
    };


    QAVPlayer(QObject *parent = nullptr);
    ~QAVPlayer();

    void setSource(const QUrl &url);
    QUrl source() const;

    bool hasAudio() const;
    bool hasVideo() const;

    State state() const;
    MediaStatus mediaStatus() const;
    qint64 duration() const;
    qint64 position() const;
    qreal speed() const;

    bool isSeekable() const;
    Error error() const;
    QString errorString() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void seek(qint64 position);
    void setSpeed(qreal rate);

Q_SIGNALS:
    void sourceChanged(const QUrl &url);
    void stateChanged(QAVPlayer::State newState);
    void mediaStatusChanged(QAVPlayer::MediaStatus status);
    void errorOccurred(QAVPlayer::Error, const QString &str);
    void durationChanged(qint64 duration);
    void seekableChanged(bool seekable);
    void speedChanged(qreal rate);
    void seeked(qint64 pos);

    void videoFrame(const QAVVideoFrame &frame);
    void audioFrame(const QAVAudioFrame &frame);

protected:
    QScopedPointer<QAVPlayerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVPlayer)
    Q_DECLARE_PRIVATE(QAVPlayer)
};

Q_DECLARE_METATYPE(QAVPlayer::State)
Q_DECLARE_METATYPE(QAVPlayer::MediaStatus)
Q_DECLARE_METATYPE(QAVPlayer::Error)

QT_END_NAMESPACE

#endif
