/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "playercontroller.h"

#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudioframe.h>
#include <QUrl>
#include <QTimer>
#include <QFileInfo>

static bool isStreamCurrent(int index, const QList<QAVStream> &streams)
{
    for (const auto &stream: streams) {
        if (stream.index() == index)
            return true;
    }
    return false;
}

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
{
    connectPlayerSignals();
    m_audioOutput.setVolume(m_volume);
}

void PlayerController::connectPlayerSignals()
{
    // Qt::DirectConnection is required: QAVPlayer emits from its internal
    // decode thread. We push straight into the sink here – QVideoSink is
    // thread-safe for setVideoFrame().
    QObject::connect(&m_player, &QAVPlayer::videoFrame,
                     this, [this](const QAVVideoFrame &frame) {
        if (m_videoSink) {
            // QAVVideoFrame is implicitly convertible to QVideoFrame.
            // When HW acceleration is active this is copy-free.
            QVideoFrame qframe = frame;
            m_videoSink->setVideoFrame(qframe);
        }
    }, Qt::DirectConnection);

    QObject::connect(&m_player, &QAVPlayer::audioFrame, this, [this](const QAVAudioFrame &frame) {
        m_audioOutput.play(frame);
    }, Qt::DirectConnection);

    QObject::connect(&m_player, &QAVPlayer::subtitleFrame, this, [this](const QAVSubtitleFrame &frame) {
        QString text;
        if (m_subtitleMuxer.parseText(frame, text) >= 0)
            emit subtitleTextChanged(text, frame.duration() * 1000);
    }, Qt::DirectConnection);

    QObject::connect(&m_player, &QAVPlayer::played, this, [this](qint64 pos) {
        Q_UNUSED(pos)
        m_playing = true;
        m_paused  = false;
        emit playingChanged();
        emit pausedChanged();
    });

    QObject::connect(&m_player, &QAVPlayer::paused, this, [this](qint64 pos) {
        Q_UNUSED(pos)
        m_playing = false;
        m_paused  = true;
        emit playingChanged();
        emit pausedChanged();
    });

    QObject::connect(&m_player, &QAVPlayer::stopped, this, [this](qint64 pos) {
        Q_UNUSED(pos)
        m_playing = false;
        m_paused  = false;
        m_position = 0;
        emit playingChanged();
        emit pausedChanged();
        emit positionChanged();
    });

    QObject::connect(&m_player, &QAVPlayer::stepped, this, [this](qint64 pos) {
        if (m_playing) {
            m_playing = false;
            m_paused  = false;
            emit playingChanged();
            emit pausedChanged();
        }
        if (pos != m_position) {
            m_position = pos;
            emit positionChanged();
        }
    });

    QObject::connect(&m_player, &QAVPlayer::seeked, this, [this](qint64 pos) {
        m_position = pos;
        emit positionChanged();
    });

    QObject::connect(&m_player, &QAVPlayer::errorOccurred, this, [this](QAVPlayer::Error, const QString &str) {
        m_errorString = str;
        emit errorOccurred();
    });

    QObject::connect(&m_player, &QAVPlayer::mediaStatusChanged,
                     this, [this](QAVPlayer::MediaStatus status) {
        if (status == QAVPlayer::LoadedMedia) {
            // Duration is available once the media is loaded
            m_duration = m_player.duration();
            m_hasMedia = true;
            emit durationChanged();
            emit hasMediaChanged();
            notifyStreams();
            m_player.play();
        } else if (status == QAVPlayer::EndOfMedia) {
            m_playing = false;
            m_paused = false;
            emit playingChanged();
            emit pausedChanged();
        } else if (status == QAVPlayer::NoMedia) {
            reset();
        }
    });

    // QAVPlayer does not emit a continuous position signal; we poll via a timer.
    auto *posTimer = new QTimer(this);
    posTimer->setInterval(1000);
    QObject::connect(posTimer, &QTimer::timeout, this, [this]() {
        qint64 pos = m_player.position();
        if (pos != m_position) {
            m_position = pos;
            emit positionChanged();
        }
    });
    posTimer->start();
}

void PlayerController::setVideoSink(QVideoSink *sink)
{
    m_videoSink = sink;
}

void PlayerController::setVolume(qreal v)
{
    v = qBound(0.0, v, 1.0);
    if (qFuzzyCompare(v, m_volume))
        return;
    m_volume = v;
    m_audioOutput.setVolume(v);
    emit volumeChanged();
}

void PlayerController::setSource(const QString &source)
{
    m_player.setSource(source);
}

void PlayerController::openFile(const QString &path)
{
    // QML FileDialog returns a file:// URL on some platforms
    QString localPath = path;
    if (localPath.startsWith(QLatin1String("file://")))
        localPath = QUrl(localPath).toLocalFile();
    setSource(localPath);
}

void PlayerController::openUrl(const QString &url)
{
    setSource(url);
}

void PlayerController::play()
{
    m_player.play();
}

void PlayerController::pause()
{
    m_player.pause();
}

void PlayerController::stop()
{
    m_player.setSource({});
    reset();
}

void PlayerController::seek(qint64 ms)
{
    m_player.seek(ms);
}

void PlayerController::stepForward()
{
    m_player.stepForward();
}

void PlayerController::stepBackward()
{
    m_player.stepBackward();
}

QStringList PlayerController::subtitleTracks() const
{
    QStringList ret;
    auto streams = m_player.availableSubtitleStreams();
    const auto lang = QString::fromLatin1("language");
    for (int i = 0; i < streams.size(); ++i) {
        auto &s = streams[i];
        auto name = "Track " + QString::number(i);
        auto md = s.metadata();
        if (md.contains(lang))
            name += " [" + md[lang] + "]";
        ret.push_back(name);
    }
    return ret;
}

void PlayerController::setSubtitleTrack(int index)
{
    auto streams = m_player.availableSubtitleStreams();
    for (int i = 0; i < streams.size(); ++i) {
        auto &s = streams[i];
        if (i == index) {
            m_player.setSubtitleStream(s);
            emit subtitleTrackChanged(index);
            return;
        }
    }
    m_player.setSubtitleStreams({});
    emit subtitleTrackChanged(-1);
}

void PlayerController::notifyStreams()
{
    emit subtitleTracksChanged();
    auto availableSubtitleStreams = m_player.availableSubtitleStreams();
    auto currentSubtitleStreams = m_player.currentSubtitleStreams();
    for (int i = 0; i < availableSubtitleStreams.size(); ++i) {
        auto &s = availableSubtitleStreams[i];
        if (isStreamCurrent(s.index(), currentSubtitleStreams)) {
            m_subtitleMuxer.unload();
            m_subtitleMuxer.load(s);
            emit subtitleTrackChanged(i);
            break;
        }
    }
}

void PlayerController::reset()
{
    m_hasMedia = false;
    m_playing = false;
    m_paused  = false;
    m_position = 0;
    m_duration = 0;
    emit hasMediaChanged();
    emit playingChanged();
    emit pausedChanged();
    emit positionChanged();
    emit durationChanged();
    emit subtitleTracksChanged();
    emit subtitleTrackChanged(-1);
}
