/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"

#include <QtTest/QtTest>

void tst_QAVPlayer::quitAudio()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
}

void tst_QAVPlayer::playIncorrectSource()
{
    QAVPlayer p;
    QSignalSpy spyStateChanged(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QCOMPARE(spyStateChanged.count(), 0);

    p.setSource("unknown");
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spyErrorOccurred.clear();

    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QCOMPARE(spyStateChanged.count(), 0);
    QCOMPARE(spyErrorOccurred.count(), 0);

    spyStateChanged.clear();

    p.setSource("unknown");
    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(spyStateChanged.count(), 0);
    QCOMPARE(spyErrorOccurred.count(), 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(spyStateChanged.count(), 1);
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
}

void tst_QAVPlayer::playAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.position(), 0);
    QVERIFY(!p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(p.isSeekable());

    spyState.clear();
    spyMediaStatus.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QTRY_VERIFY(p.position() != 0);
    QCOMPARE(spyMediaStatus.count(), 0);

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

    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
}

void tst_QAVPlayer::playAudioOutput()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    QAVAudioFrame frame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frame = f; }, Qt::DirectConnection);
    p.play();

    QTRY_VERIFY(p.position() != 0);
    QTRY_VERIFY(frame);
    QCOMPARE(frame.format().sampleFormat(), QAVAudioFormat::Int16);

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

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());
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

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());
    p.play();

    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(p.position() != 0);

    p.stop();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() != 0);
    QTRY_VERIFY(p.duration() != 0);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

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

void tst_QAVPlayer::speedAudio()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    p.setSpeed(0.5);
    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.speed(), 0.5);
    QTest::qWait(50);
    p.setSpeed(2.0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::audioPositionWithCover()
{
    QAVPlayer p;
    qint64 pos = 0;
    QAVAudioFrame frame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { pos = p.position(); frame = f; }, Qt::DirectConnection);

    QFileInfo file(testData("test.mp3"));
    p.setSource(file.absoluteFilePath());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    p.play();
    p.setSynced(false);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_VERIFY(frame);
    QVERIFY(pos > 0);
}
