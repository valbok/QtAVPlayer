/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVPLAYER_H
#define QAVPLAYER_H

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

#include <QtAVPlayer/private/qtavplayerglobal_p.h>
#include <QMediaPlayer>
#include <QUrl>
#include <QScopedPointer>
#include <QObject>

QT_BEGIN_NAMESPACE

class QAVPlayerPrivate;
class QAbstractVideoSurface;
class Q_AVPLAYER_EXPORT QAVPlayer : public QObject
{
    Q_OBJECT
public:
    QAVPlayer(QObject *parent = nullptr);
    ~QAVPlayer();

    void setSource(const QUrl &url);
    QUrl source() const;

    bool isAudioAvailable() const;
    bool isVideoAvailable() const;

    void setVideoSurface(QAbstractVideoSurface *surface);

    QMediaPlayer::State state() const;
    QMediaPlayer::MediaStatus mediaStatus() const;
    qint64 duration() const;
    qint64 position() const;
    int volume() const;
    bool isMuted() const;
    qreal speed() const;

    bool isSeekable() const;
    QMediaPlayer::Error error() const;
    QString errorString() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void seek(qint64 position);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setSpeed(qreal rate);

Q_SIGNALS:
    void sourceChanged(const QUrl &url);
    void stateChanged(QMediaPlayer::State newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void errorOccurred(QMediaPlayer::Error, const QString &str);
    void durationChanged(qint64 duration);
    void seekableChanged(bool seekable);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void speedChanged(qreal rate);

protected:
    QScopedPointer<QAVPlayerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVPlayer)
    Q_DECLARE_PRIVATE(QAVPlayer)
};

QT_END_NAMESPACE

#endif
