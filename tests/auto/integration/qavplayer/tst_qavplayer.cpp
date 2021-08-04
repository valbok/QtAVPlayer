/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"

#include <QDebug>
#include <QtTest/QtTest>

QT_USE_NAMESPACE

class tst_QAVPlayer : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();

private slots:
    void construction();
    void sourceChanged();
    void speedChanged();
    void quitAudio();
    void playIncorrectSource();
    void playAudio();
    void playAudioOutput();
    void pauseAudio();
    void stopAudio();
    void seekAudio();
    void speedAudio();
    void playVideo();
    void pauseVideo();
    void seekVideo();
    void speedVideo();
    void videoFrame();
    void pauseSeekVideo();
    void videoFiles_data();
    void videoFiles();
};

void tst_QAVPlayer::initTestCase()
{
    QThreadPool::globalInstance()->setMaxThreadCount(20);
}

void tst_QAVPlayer::construction()
{
    QAVPlayer p;
    QVERIFY(p.source().isEmpty());
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.speed(), 1.0);
    QVERIFY(!p.isSeekable());
    QCOMPARE(p.error(), QAVPlayer::NoError);
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

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
}

void tst_QAVPlayer::playIncorrectSource()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::stateChanged);

    p.play();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QCOMPARE(spy.count(), 0);

    p.setSource(QUrl(QLatin1String("unknown")));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.error(), QAVPlayer::ResourceError);
    QVERIFY(!p.errorString().isEmpty());

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.error(), QAVPlayer::ResourceError);
    QVERIFY(!p.errorString().isEmpty());
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QCOMPARE(spy.count(), 0);

    p.setSource(QUrl(QLatin1String("unknown")));
    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(spy.count(), 0);
}

void tst_QAVPlayer::playAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
    QVERIFY(p.hasAudio());
    QVERIFY(!p.hasVideo());
    QVERIFY(p.isSeekable());

    spyState.clear();
    spyMediaStatus.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 0);

    QTRY_VERIFY(p.position() != 0);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> Loaded
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY(p.position() != 0);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
}

void tst_QAVPlayer::playAudioOutput()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QAVAudioFrame frame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frame = f; });
    p.play();

    QTRY_VERIFY(p.position() != 0);
    QCOMPARE(frame.format().sampleFormat(), QAVAudioFormat::Int32);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
}

void tst_QAVPlayer::pauseAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);

    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);

    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(spyState.count(), 2); // Stopped -> Playing -> Paused
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QTRY_COMPARE(spyPaused.count(), 1);

    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 1);
}

void tst_QAVPlayer::stopAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();

    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(p.position() != 0);

    p.stop();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.position() != 0);
    QVERIFY(p.duration() != 0);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::seekAudio()
{
    QAVPlayer p;
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(500);
    QTRY_COMPARE(p.position(), 500);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(p.position() >= 500);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(100);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 100);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.position(), p.duration());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);

    p.seek(100000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(200);
    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() > 200);

    p.seek(100);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() < 200);

    p.seek(p.duration());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);
}

void tst_QAVPlayer::speedAudio()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/test.wav"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.setSpeed(0.5);
    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.speed(), 0.5);
    QTest::qWait(50);
    p.setSpeed(2.0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::playVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);
    QSignalSpy spyVideoFrameRateChanged(&p, &QAVPlayer::videoFrameRateChanged);

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(!p.hasAudio());
    QVERIFY(!p.hasVideo());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 0);
    QCOMPARE(p.videoFrameRate(), 0.0);

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 1);
    QCOMPARE(p.videoFrameRate(), 0.04);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());
    QVERIFY(p.hasAudio());
    QVERIFY(p.hasVideo());
    QVERIFY(p.isSeekable());
    QCOMPARE(p.position(), 0);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 1);

    QTRY_VERIFY(p.position() != 0);
}

void tst_QAVPlayer::pauseVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
}

void tst_QAVPlayer::seekVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame, &framesCount](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    p.seek(14500);
    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_VERIFY(frame == false);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
    QVERIFY(p.hasVideo());
    QVERIFY(p.hasAudio());
    QCOMPARE(spySeeked.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spySeeked.clear();

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(frame == true);
    QVERIFY(framesCount > 0);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QTRY_COMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> LoadedMedia
    QCOMPARE(spyDuration.count(), 0);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 80); // Playing from beginning
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QCOMPARE(p.duration(), 15019);
    QCOMPARE(p.error(), QAVPlayer::NoError);
    QVERIFY(p.errorString().isEmpty());

    QTRY_VERIFY(p.position() < 5000 && p.position() > 1000);

    spyMediaStatus.clear();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(frame == false);
    QVERIFY(p.position() != p.duration());
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spySeeked.clear();
    framesCount = 0;

    p.setSource({});
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // Loaded -> NoMedia
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spyState.clear();

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == true);
    QVERIFY(framesCount > 0);

    p.seek(14500);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spySeeked.count(), 1);
    QVERIFY(seekPosition >= 0);
    QTRY_VERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(p.position() != p.duration());
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == true);

    framesCount = 0;
    QTest::qWait(10);

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == false);
    QTRY_COMPARE(seekPosition, 0);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(0);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(seekPosition < 10);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    const int pos = p.position();
    QTest::qWait(50);
    QCOMPARE(p.position(), pos);
    p.seek(-1);
    QCOMPARE(p.position(), pos);
    QTest::qWait(50);
    QCOMPARE(p.position(), pos);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spySeeked.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    spySeeked.clear();

    p.seek(5000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    p.play();

    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 5000, 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 5000) < 500);
    seekPosition = -1;

    p.seek(8000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 8000, 10000);
    QCOMPARE(spySeeked.count(), 2);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 8000) < 500);
    seekPosition = -1;

    p.seek(2000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 2000);
    QTRY_COMPARE(spySeeked.count(), 3);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 2000) < 500);
    QTRY_VERIFY(frame == true);
    seekPosition = -1;
    framesCount = 0;

    p.stop();
    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == false);
    spyPaused.clear();

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_COMPARE(spyPaused.count(), 1);
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    spyPaused.clear();
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    p.pause();
    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    p.seek(14500);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);

    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QCOMPARE(spySeeked.count(), 4);
    QCOMPARE(seekPosition, -1);
    QCOMPARE(spyPaused.count(), 0);
    p.play();
}

void tst_QAVPlayer::speedVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.setSpeed(5);
    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
    QTRY_VERIFY(!frame);

    p.setSpeed(0.5);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.speed(), 0.5);

    p.play();

    QTest::qWait(100);
    QTRY_VERIFY(frame);
    p.setSpeed(5);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(frame);

    p.play();
}

void tst_QAVPlayer::videoFrame()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.play();
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.stop();
    QTRY_VERIFY(frame.size().isEmpty());
}

void tst_QAVPlayer::pauseSeekVideo()
{
    QAVPlayer p;

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; if (f) ++framesCount; });
    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });
    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(frame.size().isEmpty());
    QCOMPARE(pausePosition, -1);

    p.pause();
    QCOMPARE(framesCount, 0);
    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;

    QCOMPARE(p.state(), QAVPlayer::PausedState);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QTest::qWait(100);
    QVERIFY(frame.size().isEmpty());
    QVERIFY(framesCount < 2);
    QCOMPARE(pausePosition, -1);

    p.pause();

    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    int count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(pausePosition, -1);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QVERIFY(seekPosition >= 0);
    QVERIFY(seekPosition < 220);
    seekPosition = -1;

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTest::qWait(100);
    QVERIFY(frame.size().isEmpty());
    QVERIFY(framesCount < 2);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(pausePosition, -1);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(count != framesCount);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(count != framesCount);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(framesCount, 0);
    QTest::qWait(100);
    QVERIFY(framesCount >= 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 1) < 100);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);
}

void tst_QAVPlayer::videoFiles_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("duration");
    QTest::addColumn<bool>("hasVideo");
    QTest::addColumn<bool>("hasAudio");

    QTest::newRow("test.wav") << QString("../testdata/test.wav") << 999 << false << true;
    QTest::newRow("colors.mp4") << QString("../testdata/colors.mp4") << 15019 << true << true;
    QTest::newRow("shots0000.dv") << QString("../testdata/shots0000.dv") << 40 << true << false;
    QTest::newRow("dv_dsf_1_stype_1.dv") << QString("../testdata/dv_dsf_1_stype_1.dv") << 600 << true << true;
    QTest::newRow("dv25_ntsc_411_169_2ch_48k_bars_sine.dv") << QString("../testdata/dv25_ntsc_411_169_2ch_48k_bars_sine.dv") << 1968 << true << true;
    QTest::newRow("dv25_ntsc_411_4-3_2ch_48k_bars_sine.dv") << QString("../testdata/dv25_ntsc_411_4-3_2ch_48k_bars_sine.dv") << 1968 << true << true;
    QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << QString("../testdata/dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << 2000 << true << true;
    QTest::newRow("dv25_pal__411_4-3_2ch_48k_bars_sine.dv") << QString("../testdata/dv25_pal__411_4-3_2ch_48k_bars_sine.dv") << 2000 << true << true;
    QTest::newRow("dv25_pal__420_4-3_2ch_48k_bars_sine.dv") << QString("../testdata/dv25_pal__420_4-3_2ch_48k_bars_sine.dv") << 2000 << true << true;
}

void tst_QAVPlayer::videoFiles()
{
    QFETCH(QString, path);
    QFETCH(int, duration);
    QFETCH(bool, hasVideo);
    QFETCH(bool, hasAudio);

    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(QUrl::fromLocalFile(file.absoluteFilePath()));

    int vf = 0;
    QAVVideoFrame videoFrame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { videoFrame = f; if (f) ++vf; });
    int af = 0;
    QAVAudioFrame audioFrame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { audioFrame = f; if (f) ++af; });

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(!videoFrame);
    QVERIFY(!audioFrame);
    QCOMPARE(p.duration(), duration);
    QCOMPARE(p.hasVideo(), hasVideo);
    QCOMPARE(p.hasAudio(), hasAudio);

    p.pause();
    if (hasVideo)
        QTRY_COMPARE(vf, 1);

    vf = 0;
    af = 0;

    p.play();
    if (hasVideo) {
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || videoFrame);
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || vf > 0);
    }

    if (hasAudio) {
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || audioFrame);
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || af > 0);
    }

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 18000);
    if (hasVideo)
        QTRY_VERIFY(!videoFrame);

    vf = 0;
    af = 0;

    p.pause();
    p.play();
    if (hasVideo) {
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || videoFrame);
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || vf > 0);
    }
    if (hasAudio) {
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || audioFrame);
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || af > 0);
    }

    videoFrame = QAVVideoFrame();

    p.pause();
    if (hasVideo)
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || videoFrame);

    p.seek(duration * 0.8);
    if (hasVideo)
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || videoFrame);

    videoFrame = QAVVideoFrame();
    p.play();
    if (hasVideo)
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || videoFrame);

    p.play();
    p.stop();
    if (hasVideo)
        QTRY_VERIFY(!videoFrame);

    p.pause();
    p.stop();
    p.pause();
    p.play();
    p.seek(duration * 0.9);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 18000);
    if (hasVideo)
        QTRY_VERIFY(!videoFrame);
}

QTEST_MAIN(tst_QAVPlayer)
#include "tst_qavplayer.moc"
