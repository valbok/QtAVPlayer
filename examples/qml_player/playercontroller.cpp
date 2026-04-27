/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "playercontroller.h"

#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudioframe.h>
#include <QtAVPlayer/qavcodec_p.h>
#include <QUrl>
#include <QTimer>
#include <QFileInfo>

extern "C" {
#include <libavcodec/avcodec.h>
}

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
}

void PlayerController::connectPlayerSignals()
{
    // Qt::DirectConnection is required: QAVPlayer emits from its internal
    // decode thread. We push straight into the sink here – QVideoSink is
    // thread-safe for setVideoFrame().
    QObject::connect(&m_player, &QAVPlayer::videoFrame,
                     this, [this](const QAVVideoFrame &frame) {
        if (m_videoSink) {
            // If the non-copy-free render is requested
            // map the frame before converting to QVideoFrame.
            // This will force rendering mapped data instead of texture handles.
            if (!m_copyFreeRender)
                frame.map();
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
        emit playingChanged();
    });

    QObject::connect(&m_player, &QAVPlayer::paused, this, [this](qint64 pos) {
        Q_UNUSED(pos)
        m_playing = false;
        emit playingChanged();
    });

    QObject::connect(&m_player, &QAVPlayer::stopped, this, [this](qint64 pos) {
        Q_UNUSED(pos)
        m_playing = false;
        m_position = 0;
        emit playingChanged();
        emit positionChanged();
    });

    QObject::connect(&m_player, &QAVPlayer::stepped, this, [this](qint64 pos) {
        if (m_playing) {
            m_playing = false;
            emit playingChanged();
        }
        if (pos != m_position) {
            m_position = pos;
            emit positionChanged();
        }
        m_audioOutput.setVolume(m_volume);
    });

    QObject::connect(&m_player, &QAVPlayer::seeked, this, [this](qint64 pos) {
        m_position = pos;
        emit positionChanged();
        m_audioOutput.setVolume(m_volume);
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
            emit playingChanged();
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
    m_player.setInputVideoCodec(m_videoCodec);
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
    m_audioOutput.setVolume(0);
    m_player.seek(ms);
}

void PlayerController::stepForward()
{
    m_audioOutput.setVolume(0);
    m_player.stepForward();
}

void PlayerController::stepBackward()
{
    m_audioOutput.setVolume(0);
    m_player.stepBackward();
}

static QStringList tracks(const QList<QAVStream> &streams)
{
    QStringList ret;
    const auto lang = QString::fromLatin1("language");
    const auto title = QString::fromLatin1("title");
    for (int i = 0; i < streams.size(); ++i) {
        auto &s = streams[i];
        auto name = "Track " + QString::number(i);
        auto md = s.metadata();
        if (md.contains(title)) {
            name = md[title];
        }
        if (md.contains(lang))
            name += " [" + md[lang] + "]";
        ret.push_back(name);
    }
    return ret;
}

QStringList PlayerController::subtitleTracks() const
{
    return tracks(m_player.availableSubtitleStreams());
}

void PlayerController::setSubtitleTrack(int index)
{
    auto streams = m_player.availableSubtitleStreams();
    if (index >= 0 && index < streams.size()) {
        m_player.setSubtitleStream(streams[index]);
        emit subtitleTrackChanged(index);
        return;
    }
    m_player.setSubtitleStreams({});
    emit subtitleTrackChanged(-1);
}

QStringList PlayerController::audioTracks() const
{
    return tracks(m_player.availableAudioStreams());
}

void PlayerController::setAudioTrack(int index)
{
    auto streams = m_player.availableAudioStreams();
    if (index >= 0 && index < streams.size()) {
        m_player.setAudioStream(streams[index]);
        emit audioTrackChanged(index);
    }
}

QStringList PlayerController::videoTracks() const
{
    QList<QAVStream> streams;
    for (const auto &s : m_player.availableVideoStreams()) {
        // Ignore posters
        if (s.framesCount())
            streams.append(s);
    }
    return tracks(streams);
}

void PlayerController::setVideoTrack(int index)
{
    auto streams = m_player.availableVideoStreams();
    if (index >= 0 && index < streams.size()) {
        m_player.setVideoStream(streams[index]);
        emit videoTrackChanged(index);
    }
}

static QPair<int, QAVStream> currentStream(const QList<QAVStream> &available, const QList<QAVStream> &current)
{
    for (int i = 0; i < available.size(); ++i) {
        auto &s = available[i];
        if (isStreamCurrent(s.index(), current)) {
            return {i, s};
        }
    }
    return {-1, {}};
}

void PlayerController::notifyStreams()
{
    emit subtitleTracksChanged();
    auto i = currentStream(m_player.availableSubtitleStreams(), m_player.currentSubtitleStreams());
    if (i.first >= 0) {
        m_subtitleMuxer.unload();
        m_subtitleMuxer.load(i.second);
        emit subtitleTrackChanged(i.first);
    }
    emit audioTracksChanged();
    i = currentStream(m_player.availableAudioStreams(), m_player.currentAudioStreams());
    if (i.first >= 0)
        emit audioTrackChanged(i.first);
    emit videoTracksChanged();
    auto videoStreams = m_player.currentVideoStreams();
    i = currentStream(m_player.availableVideoStreams(), videoStreams);
    if (i.first >= 0) {
        emit videoTrackChanged(i.first);
        if (m_videoCodec != "software") {
            auto avctx = videoStreams[0].codec()->avctx();
            emit hwDeviceChanged(avctx && avctx->hw_device_ctx);
        }
    }
}

void PlayerController::reset()
{
    m_hasMedia = false;
    m_playing = false;
    m_position = 0;
    m_duration = 0;
    emit hasMediaChanged();
    emit playingChanged();
    emit positionChanged();
    emit durationChanged();
    emit subtitleTracksChanged();
    emit subtitleTrackChanged(-1);
    emit audioTracksChanged();
    emit videoTracksChanged();
}

void PlayerController::setVideoCodec(const QString &codec)
{
    m_videoCodec = codec;
    m_player.setInputVideoCodec(codec);
    // Since the codec is applied during the loading,
    // it requires to reload the source.
    auto source = m_player.source();
    m_position = m_player.position();
    m_audioOutput.setVolume(0);
    m_player.setSource({});
    m_player.setSource(source);
    m_player.seek(m_position);
    // Selection of tracks are now lost
}

void PlayerController::setCopyFreeRender(bool copyfree)
{
    m_copyFreeRender = copyfree;
}
