/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#pragma once

#include <QObject>
#include <QtAVPlayer/qavplayer.h>
#include <QtAVPlayer/qavaudiooutput.h>
#include <QtAVPlayer/qavmuxer.h>
#include <QVideoSink>
#include <QVideoFrame>
#include <QString>
#include <qqml.h>

/**
 * PlayerController
 *
 * Thin bridge between QAVPlayer (C++) and the QML UI.
 * Exposes playback controls, position, duration and media status
 * as Q_PROPERTYs and slots callable from QML.
 *
 * Video frames are pushed directly into a QVideoSink which is
 * consumed by a QML VideoOutput element (copy-free when HW accel
 * is active and the Qt render backend supports it).
 */
class PlayerController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
    Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume   NOTIFY volumeChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia NOTIFY hasMediaChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorOccurred)

    Q_PROPERTY(QStringList subtitleTracks READ subtitleTracks NOTIFY subtitleTracksChanged)
    Q_PROPERTY(QStringList audioTracks READ audioTracks NOTIFY audioTracksChanged)
    Q_PROPERTY(QStringList videoTracks READ videoTracks NOTIFY videoTracksChanged)

public:
    explicit PlayerController(QObject *parent = nullptr);

    // Property accessors
    bool isPlaying() const { return m_playing; }
    bool isPaused() const { return m_paused; }
    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    qreal volume() const { return m_volume; }
    bool hasMedia() const { return m_hasMedia; }
    QString errorString() const { return m_errorString; }
    QStringList subtitleTracks() const;
    QStringList audioTracks() const;
    QStringList videoTracks() const;

    void setVolume(qreal v);

public slots:
    // Called from QML
    void setVideoSink(QVideoSink *sink);
    void openFile(const QString &path);
    void openUrl(const QString &url);
    void play();
    void pause();
    void stop();
    void seek(qint64 ms);
    void stepForward();
    void stepBackward();
    void setSubtitleTrack(int index);
    void setAudioTrack(int index);
    void setVideoTrack(int index);

signals:
    void playingChanged();
    void pausedChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void hasMediaChanged();
    void errorOccurred();
    void subtitleTracksChanged();
    void subtitleTrackChanged(int index);
    void subtitleTextChanged(const QString &text, int duration);
    void audioTracksChanged();
    void audioTrackChanged(int index);
    void videoTracksChanged();
    void videoTrackChanged(int index);

private:
    void connectPlayerSignals();
    void setSource(const QString &source);
    void notifyStreams();
    void reset();

    QAVPlayer m_player;
    QAVAudioOutput m_audioOutput;
    QVideoSink *m_videoSink = nullptr;
    QAVMuxerSubtitleFrames m_subtitleMuxer;

    bool m_playing = false;
    bool m_paused = false;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    qreal m_volume = 1.0;
    bool m_hasMedia = false;
    QString m_errorString;
};
