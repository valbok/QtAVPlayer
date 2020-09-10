/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"
#include <qabstractvideosurface.h>
#include <QtMultimedia/private/qtmultimedia-config_p.h>
#include <QDebug>
#include <QtTest/QtTest>

QT_USE_NAMESPACE

class tst_QAVPlayer : public QObject
{
    Q_OBJECT
private slots:
    void construction();
    void sourceChanged();
    void volumeChanged();
    void mutedChanged();
    void speedChanged();
    void quitAudio();
    void playIncorrectSource();
    void playAudio();
    void pauseAudio();
    void volumeAudio();
    void stopAudio();
    void seekAudio();
    void speedAudio();
    void playVideo();
    void pauseVideo();
    void seekVideo();
    void speedVideo();
    void surfaceVideo();
    void surfaceVideoUnsupported();
};

void tst_QAVPlayer::construction()
{
    QAVPlayer p;
    QVERIFY(p.source().isEmpty());
    QVERIFY(!p.isAudioAvailable());
    QVERIFY(!p.isVideoAvailable());
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.volume(), 100);
    QVERIFY(!p.isMuted());
    QCOMPARE(p.speed(), 1.0);
    QVERIFY(!p.isSeekable());
    QCOMPARE(p.error(), QMediaPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
}

void tst_QAVPlayer::sourceChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::sourceChanged);
    p.setSource(QUrl(QLatin1String("unknown.mp4")));
    QCOMPARE(spy.count(), 1);
    p.setSource(QUrl(QLatin1String("unknown.mp4")));
    QCOMPARE(spy.count(), 1);
}

void tst_QAVPlayer::volumeChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::volumeChanged);
    QCOMPARE(p.volume(), 100);
    p.setVolume(10);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.volume(), 10);
    p.setVolume(0);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(p.volume(), 0);
    p.setVolume(-10);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(p.volume(), 0);
    p.setVolume(200);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(p.volume(), 0);
    p.setVolume(100);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(p.volume(), 100);
}

void tst_QAVPlayer::mutedChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::mutedChanged);
    QVERIFY(!p.isMuted());
    p.setMuted(true);
    QCOMPARE(spy.count(), 1);
    QVERIFY(p.isMuted());
}

void tst_QAVPlayer::speedChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::speedChanged);
    QCOMPARE(p.speed(), 1.0);
    p.setSpeed(0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.speed(), 0.0);
    p.setSpeed(2);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(p.speed(), 2.0);
}

void tst_QAVPlayer::quitAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
}

void tst_QAVPlayer::playIncorrectSource()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::stateChanged);

    p.play();

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QVERIFY(!p.isAudioAvailable());
    QVERIFY(!p.isVideoAvailable());
    QCOMPARE(spy.count(), 0);

    p.setSource(QUrl(QLatin1String("unknown")));

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::InvalidMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.error(), QMediaPlayer::ResourceError);
    QVERIFY(!p.errorString().isEmpty());

    p.play();

    QCOMPARE(p.mediaStatus(), QMediaPlayer::InvalidMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.error(), QMediaPlayer::ResourceError);
    QVERIFY(!p.errorString().isEmpty());
    QVERIFY(!p.isAudioAvailable());
    QVERIFY(!p.isVideoAvailable());
    QCOMPARE(spy.count(), 0);

    p.setSource(QUrl(QLatin1String("unknown")));
    p.play();
    QCOMPARE(p.mediaStatus(), QMediaPlayer::InvalidMedia);
    QCOMPARE(spy.count(), 0);
}

void tst_QAVPlayer::playAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QVERIFY(!p.isAudioAvailable());
    QVERIFY(!p.isVideoAvailable());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.error(), QMediaPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
    QVERIFY(p.isAudioAvailable());
    QVERIFY(!p.isVideoAvailable());
    QVERIFY(p.isSeekable());

    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 2);

    QTRY_VERIFY(p.position() != 0);

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.play();

    QTRY_COMPARE(p.state(), QMediaPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> LoadedMedia
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.error(), QMediaPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY(p.position() != 0);

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);

    p.play();

    QCOMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
}

void tst_QAVPlayer::pauseAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.play();

    QTest::qWait(50);
    p.pause();

    QCOMPARE(p.state(), QMediaPlayer::PausedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 2); // Stopped -> Playing -> Paused
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded

    p.play();

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
}

void tst_QAVPlayer::volumeAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.setVolume(0);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.volume(), 0);

    p.setVolume(50);
    p.play();

    QTest::qWait(500);
    p.setVolume(100);

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.volume(), 100);
}

void tst_QAVPlayer::stopAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(p.position() != 0);

    p.stop();

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(p.position() != 0);
    QVERIFY(p.duration() != 0);

    p.play();

    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
}

void tst_QAVPlayer::seekAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(500);
    QCOMPARE(p.position(), 500);
    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QVERIFY(p.position() >= 500);
    QTRY_COMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    p.seek(100);
    QCOMPARE(p.position(), 100);
    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(p.position(), p.duration());

    p.seek(100000);
    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(200);
    p.play();
    QTRY_VERIFY(p.position() > 200);

    p.seek(100);
    QTRY_VERIFY(p.position() < 200);

    p.seek(p.duration());
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
}

void tst_QAVPlayer::speedAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.setSpeed(0.5);
    p.play();
    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QCOMPARE(p.speed(), 0.5);
    p.setSpeed(2.0);
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
}

void tst_QAVPlayer::playVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QVERIFY(!p.isAudioAvailable());
    QVERIFY(!p.isVideoAvailable());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QMediaPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
    QVERIFY(p.isAudioAvailable());
    QVERIFY(p.isVideoAvailable());
    QVERIFY(p.isSeekable());
    QCOMPARE(p.position(), 0);

    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing

    QTRY_VERIFY(p.position() != 0);
}

void tst_QAVPlayer::pauseVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);

    p.play();
    QCOMPARE(p.state(), QMediaPlayer::PlayingState);

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QMediaPlayer::PausedState);
}

void tst_QAVPlayer::seekVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(14500);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> LoadedMedia
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QMediaPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY(p.position() < 5000 && p.position() > 1000);

    p.stop();

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(p.position() != p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.setSource({});

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> NoMedia
    QCOMPARE(spyDuration.count(), 1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(spyMediaStatus.count(), 2); // NoMedia -> Loading -> Loaded
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyDuration.count(), 1);

    p.seek(14500);

    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);

    p.play();
    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(p.position() != p.duration());

    QTest::qWait(10);
    p.stop();
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);

    p.seek(0);

    QCOMPARE(p.position(), 0);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);

    p.seek(-1);

    QCOMPARE(p.position(), 0);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);

    p.seek(5000);
    p.play();

    QTRY_VERIFY(p.position() > 5000);

    p.seek(13000);
    QTRY_VERIFY(p.position() > 13000);

    p.seek(2000);
    QCOMPARE(p.position(), 2000);

    QTest::qWait(50);
    p.pause();
    p.seek(14500);
    QCOMPARE(p.state(), QMediaPlayer::PausedState);

    p.play();
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::EndOfMedia);
    p.play();
}

void tst_QAVPlayer::speedVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.setSpeed(5);
    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QMediaPlayer::EndOfMedia, 10000);

    p.setSpeed(0.5);
    QCOMPARE(p.state(), QMediaPlayer::StoppedState);
    QCOMPARE(p.speed(), 0.5);

    p.play();

    QTest::qWait(100);
    p.setSpeed(5);
    p.pause();
    QCOMPARE(p.state(), QMediaPlayer::PausedState);

    p.play();
}

class Surface : public QAbstractVideoSurface
{
public:
    Surface(const QList<QVideoFrame::PixelFormat> &f): formats(f) {}
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
        QAbstractVideoBuffer::HandleType) const override
    {
        return formats;
    }

    bool present(const QVideoFrame &f) override
    {
        if (f.isValid() && supportedPixelFormats(f.handleType()).contains(f.pixelFormat()))
            ++frames;
        return true;
    }

    int frames = 0;
    QList<QVideoFrame::PixelFormat> formats;
};

void tst_QAVPlayer::surfaceVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    Surface s({QVideoFrame::Format_YUV420P});
    p.setVideoSurface(&s);

    QCOMPARE(p.state(), QMediaPlayer::StoppedState);

    p.play();
    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_VERIFY(s.frames > 0);
}

void tst_QAVPlayer::surfaceVideoUnsupported()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();

    Surface s({QVideoFrame::Format_Invalid});
    p.setVideoSurface(&s);

    QCOMPARE(p.state(), QMediaPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QMediaPlayer::LoadedMedia);
    QVERIFY(s.frames == 0);
}

QTEST_MAIN(tst_QAVPlayer)
#include "tst_qavplayer.moc"
