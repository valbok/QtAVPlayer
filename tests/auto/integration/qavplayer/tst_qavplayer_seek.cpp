/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"

#include <QtTest/QtTest>

void tst_QAVPlayer::seekAudio()
{
    QAVPlayer p;
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    p.seek(500);
    QTRY_COMPARE(p.position(), 500);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(p.position() >= 500);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(100);
    QCOMPARE(p.position(), 100);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

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
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() > 200);

    p.seek(100);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() < 200);

    p.seek(p.duration());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);
}

void tst_QAVPlayer::seekVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);
    QSignalSpy spyStopped(&p, &QAVPlayer::stopped);

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame, &framesCount](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    p.seek(14500);
    p.play();
    QTRY_COMPARE(spyStopped.count(), 1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
    QVERIFY(!p.availableVideoStreams().isEmpty());
    QVERIFY(!p.availableAudioStreams().isEmpty());
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(spyPaused.count(), 0);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spySeeked.clear();
    spyStopped.clear();

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(frame == true);
    QVERIFY(framesCount > 0);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> Loaded
    QCOMPARE(spyDuration.count(), 0);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 80); // Playing from beginning
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QVERIFY(qAbs(p.duration() - 15019) < 2);

    QTRY_VERIFY(p.position() < 5000 && p.position() > 1000);

    spyMediaStatus.clear();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.position() != p.duration());
    QCOMPARE(spyPaused.count(), 0);
    QTRY_COMPARE(spyStopped.count(), 1);
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
    spyStopped.clear();

    p.setSource(file.absoluteFilePath());
    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(spyMediaStatus.count(), 1); // NoMeida -> Loaded
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QTRY_COMPARE(spyDuration.count(), 1);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyStopped.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == true);
    QTRY_VERIFY(framesCount > 0);

    spyStopped.clear();

    p.seek(14500);
    QTRY_COMPARE(spyStopped.count(), 1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
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
    spyStopped.clear();

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_COMPARE(spyStopped.count(), 1);
    QTRY_COMPARE(seekPosition, 0);

    spySeeked.clear();
    spyStopped.clear();
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
    p.seek(0);
    QCOMPARE(p.position(), pos);
    QTest::qWait(50);
    QCOMPARE(p.position(), pos);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(spySeeked.count(), 2);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    spySeeked.clear();

    p.seek(5000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    p.play();

    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 5000, 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QTRY_VERIFY(qAbs(seekPosition - 5000) < 500);
    seekPosition = -1;

    p.seek(8000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 8000, 10000);
    QTRY_COMPARE(spySeeked.count(), 2);
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
    QTRY_COMPARE(spyStopped.count(), 1);
    QCOMPARE(pausePosition, -1);

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

    spyStopped.clear();

    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(spyStopped.count(), 1, 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spySeeked.count(), 4);
    QCOMPARE(seekPosition, -1);
    QCOMPARE(spyPaused.count(), 0);
    p.play();
}

void tst_QAVPlayer::seekVideoNegative()
{
    QAVPlayer p;
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame, &framesCount](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    p.seek(-1000);
    QCOMPARE(p.position(), -1000);

    p.play();
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(qAbs(seekPosition - 14400) < 500);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-50000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(p.duration() - 5000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition > 0);
    qint64 sp = seekPosition;

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-5000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(qAbs(seekPosition - sp) < 1000);

    spySeeked.clear();
    seekPosition = -1;

    p.pause();
    p.seek(0);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-1000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 13500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-2000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 2500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-20000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.stop();
    p.seek(-3000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 3500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-14000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 14500);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-500);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    spySeeked.clear();
    seekPosition = -1;

    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(seekPosition, -1);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(0);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    p.stop();

    spySeeked.clear();
    seekPosition = -1;

    p.seek(p.duration());
    p.play();
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_VERIFY(qAbs(seekPosition - p.duration()) < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(p.duration() + 1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(seekPosition, -1);
}

void tst_QAVPlayer::pauseSeekVideo()
{
    QAVPlayer p;

    QFileInfo file(testData("colors.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; if (f) ++framesCount; });
    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });
    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    p.setSource(file.absoluteFilePath());
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
    QVERIFY(framesCount - count < 5);
    QTRY_VERIFY(frame);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(seekPosition < 220);
    seekPosition = -1;

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTest::qWait(100);
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
    QVERIFY(framesCount - count < 5);
    QVERIFY(frame);
    QTRY_VERIFY(seekPosition >= 0);
    QTRY_VERIFY(qAbs(seekPosition - 1) < 200);
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
    QVERIFY(framesCount - count < 5);
    QVERIFY(frame);
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
    QTRY_VERIFY(count != framesCount);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QTRY_VERIFY(count != framesCount);
    QTRY_VERIFY(seekPosition > 0);
    QTRY_VERIFY(seekPosition < 1500);
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
    QVERIFY(framesCount - count < 5);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    QTest::qWait(100);
    QTRY_VERIFY(frame);
    QTRY_VERIFY(seekPosition > 0);
    QTRY_VERIFY(seekPosition < 1500);
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
    QTRY_VERIFY(framesCount - count < 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QTRY_VERIFY(seekPosition > 0);
    QTRY_VERIFY(seekPosition < 1500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);
}

void tst_QAVPlayer::stepForward()
{
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });
    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });
    qint64 stepPosition = -1;
    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64 pos) { stepPosition = pos; });

    p.setSource(file.absoluteFilePath());
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(!frame);
    QCOMPARE(pausePosition, -1);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(!frame);
    QCOMPARE(framesCount, 0);
    QTest::qWait(50);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(frame == false);
    QCOMPARE(framesCount, 0);
    QCOMPARE(stepPosition, -1);

    qint64 prev = -1;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTest::qWait(50);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTest::qWait(50);
    QTRY_VERIFY(frame);
    QTRY_VERIFY(stepPosition > prev);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    framesCount = 0;
    stepPosition = -1;

    p.stop();
    QVERIFY(framesCount < 3);
    QCOMPARE(stepPosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;
    stepPosition = -1;

    QTest::qWait(50);
    p.stop();
    QCOMPARE(stepPosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64) { p.stepForward(); });
    bool eof = false;
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, &p, [&](QAVPlayer::MediaStatus s) { if (!eof) eof = s == QAVPlayer::EndOfMedia; });
    p.stepForward();
    QTRY_VERIFY_WITH_TIMEOUT(eof, 15000);
}

void tst_QAVPlayer::stepBackward()
{
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });
    qint64 stepPosition = -1;
    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64 pos) { stepPosition = pos; });
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);

    p.setSource(file.absoluteFilePath());
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(!frame);

    qint64 prev = 2500;
    p.seek(prev);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;

    qint64 duration = p.videoFrameRate() * 1000;
    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2466);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2433);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2400);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2366);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2333);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    spySeeked.clear();
    framesCount = 0;
    prev = 100;

    p.seek(prev);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 66);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 33);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 0);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5500);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5466);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5433);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5400);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    spySeeked.clear();
    prev = 100;
    p.seek(prev);
    QTRY_COMPARE(spySeeked.count(), 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;
    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64) { p.stepBackward(); });
    p.stepForward();
    QTRY_COMPARE(stepPosition, 0);
    QVERIFY(stepPosition < p.duration());
}

void tst_QAVPlayer::accurateSeek_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("colors.mp4") << testData("colors.mp4");
    QTest::newRow("small.mp4") << testData("small.mp4");
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov");
    QTest::newRow("test6g100.mkv") << testData("test6g100.mkv");
}

void tst_QAVPlayer::accurateSeek()
{
    QFETCH(QString, path);
    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(file.absoluteFilePath());

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    p.seek(5000);
    QTRY_COMPARE(seekPosition, 5000);
    QTRY_VERIFY(framesCount < 3);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(frame.pts(), 5.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(4000);
    QTRY_COMPARE(seekPosition, 4000);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 4.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(3000);
    QTRY_COMPARE(seekPosition, 3000);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 3.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(2000);
    QTRY_COMPARE(seekPosition, 2000);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 2.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QTRY_COMPARE(seekPosition, 1000);
    QTRY_VERIFY(framesCount < 3);
    QCOMPARE(frame.pts(), 1.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(0);
    QTRY_COMPARE(seekPosition, 0);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 0.0);

    seekPosition = -1;
    frame = QAVVideoFrame();

    p.play();
    p.seek(2000);
    QTRY_COMPARE(seekPosition, 2000);

    p.seek(1000);
    QTRY_COMPARE(seekPosition, 1000);

    p.seek(3000);
    QTRY_COMPARE(seekPosition, 3000);

    p.seek(5000);
    QTRY_COMPARE(seekPosition, 5000);

    p.seek(4000);
    QTRY_COMPARE(seekPosition, 4000);

    QTest::qWait(100);

    p.seek(0);
    QTRY_COMPARE(seekPosition, 0);

    QTest::qWait(100);

    p.seek(1500);
    QTRY_VERIFY(qAbs(seekPosition - 1500) < 30);

    QTest::qWait(100);

    p.seek(4500);
    QTRY_VERIFY(qAbs(seekPosition - 4500) < 30);

    QTest::qWait(100);

    p.seek(3500);
    QTRY_VERIFY(qAbs(seekPosition - 3500) < 30);

    p.seek(2500);
    QTRY_VERIFY(qAbs(seekPosition - 2500) < 30);

    p.pause();

    p.seek(1600);
    QTRY_VERIFY(qAbs(seekPosition - 1600) < 30);

    p.seek(2600);
    QTRY_VERIFY(qAbs(seekPosition - 2600) < 30);

    p.seek(4600);
    QTRY_VERIFY(qAbs(seekPosition - 4600) < 30);

    QTest::qWait(100);

    p.seek(500);
    QTRY_VERIFY(qAbs(seekPosition - 500) < 30);

    p.seek(0);
    QTRY_COMPARE(seekPosition, 0);

    seekPosition = INT_MIN;

    p.play();
    p.seek(1234);
    QTRY_VERIFY(qAbs(seekPosition - 1234) < 50);

    seekPosition = INT_MIN;

    p.seek(2345);
    QTRY_VERIFY(qAbs(seekPosition - 2345) < 50);

    seekPosition = INT_MIN;

    p.seek(5321);
    QTRY_VERIFY(qAbs(seekPosition - 5321) < 50);

    seekPosition = INT_MIN;

    p.seek(100);
    QTRY_VERIFY(qAbs(seekPosition - 100) < 50);

    seekPosition = INT_MIN;

    p.seek(200);
    QTRY_VERIFY(qAbs(seekPosition - 200) < 50);

    seekPosition = INT_MIN;

    p.seek(999);
    QTRY_VERIFY(qAbs(seekPosition - 999) < 50);

    seekPosition = INT_MIN;

    p.seek(123);
    QTRY_VERIFY(qAbs(seekPosition - 123) < 50);

    seekPosition = INT_MIN;

    p.seek(321);
    QTRY_VERIFY(qAbs(seekPosition - 321) < 50);

    seekPosition = INT_MIN;

    p.seek(666);
    QTRY_VERIFY(qAbs(seekPosition - 666) < 50);

    seekPosition = INT_MIN;

    p.seek(10);
    QTRY_VERIFY(qAbs(seekPosition - 10) < 50);

    seekPosition = INT_MIN;

    p.seek(1);
    QTRY_VERIFY(qAbs(seekPosition - 1) < 50);

    seekPosition = INT_MIN;

    p.seek(p.duration() - 1000);
    QTRY_VERIFY(qAbs(seekPosition - p.duration() + 1000) < 50);

    seekPosition = INT_MIN;

    p.seek(-1000);
    QTRY_VERIFY(qAbs(seekPosition - p.duration() + 1000) < 50);

    seekPosition = INT_MIN;

    p.seek(p.duration());
    QTRY_VERIFY(qAbs(seekPosition - p.duration()) < 100);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::lastFrame()
{
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));
    p.setSource(file.absoluteFilePath());

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    p.play();
    p.seek(100000);

    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QCOMPARE(seekPosition, 5500);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);

    framesCount = 0;
    seekPosition = -1;

    p.seek(0);
    QTRY_COMPARE(framesCount, 1);

    p.play();
    framesCount = 0;
    seekPosition = -1;

    p.seek(p.duration());
    QTRY_COMPARE(framesCount, 1);
    QVERIFY(frame);
    QTRY_COMPARE(seekPosition, 5500);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}
