/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"

#include <QtTest/QtTest>
#include <QDebug>

void tst_QAVPlayer::bsf()
{
    QAVPlayer p;
    QFileInfo file(testData("test.mov"));
    p.setSource(file.absoluteFilePath());

    QSignalSpy spy(&p, &QAVPlayer::bitstreamFilterChanged);

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.setBitstreamFilter("noise");
    p.play();
    QTRY_COMPARE(spy.count(), 1);
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);

    spy.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("noise");
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);

    spy.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("");
    QTRY_COMPARE(spy.count(), 1);
    p.setSource("");
    p.setSource(file.absoluteFilePath());

    spy.clear();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.bitstreamFilter().isEmpty());

    p.setBitstreamFilter("noise");
    p.play();

    QVERIFY(!p.bitstreamFilter().isEmpty());
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);

    spy.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("noise");
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::bsfInvalid()
{
    QAVPlayer p;
    QFileInfo file(testData("test.mov"));
    p.setSource(file.absoluteFilePath());

    QSignalSpy spy(&p, &QAVPlayer::bitstreamFilterChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.setBitstreamFilter("obey=666");
    p.play();

    QTRY_COMPARE(spy.count(), 1);
    QTRY_VERIFY(spyErrorOccurred.count() > 0);

    QCOMPARE(framesCount, 0);
    QVERIFY(!frame);

    spy.clear();
    spyErrorOccurred.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("");
    QTRY_COMPARE(spy.count(), 1);
    p.setSource("");
    p.setSource(file.absoluteFilePath());

    spy.clear();
    spyErrorOccurred.clear();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.bitstreamFilter().isEmpty());

    p.setBitstreamFilter("makeluv=69");
    p.play();

    QVERIFY(!p.bitstreamFilter().isEmpty());
    QTRY_COMPARE(spyErrorOccurred.count(), 1);
    QCOMPARE(framesCount, 0);
    QVERIFY(!frame);

    spyErrorOccurred.clear();

    p.setBitstreamFilter("");
    p.play();
    QTRY_VERIFY(frame);
    QVERIFY(framesCount > 0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.setBitstreamFilter("not=war");
    p.play();

    QTRY_COMPARE(spyErrorOccurred.count(), 1);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);

    spy.clear();
    spyErrorOccurred.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("noise");
    p.play();

    QTRY_VERIFY(frame);
    QVERIFY(framesCount > 0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spyErrorOccurred.count(), 0);
}

void tst_QAVPlayer::inputFormat()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::inputFormatChanged);
    QCOMPARE(p.inputFormat(), "");
    p.setInputFormat("v4l2");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.inputFormat(), "v4l2");
}

void tst_QAVPlayer::inputVideoCodec()
{
    QAVPlayer p;
    QFileInfo file(testData("small.mp4"));
    QSignalSpy spy(&p, &QAVPlayer::inputVideoCodecChanged);
    p.setSource(file.absoluteFilePath());
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &) { ++framesCount; });

    QCOMPARE(p.inputVideoCodec(), "");
    p.setInputVideoCodec("h264");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.inputVideoCodec(), "h264");
    QVERIFY(!QAVPlayer::supportedVideoCodecs().isEmpty());
    if (!QAVPlayer::supportedVideoCodecs().contains("h264"))
        return;

    p.setSynced(false);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QTRY_COMPARE(framesCount, 166);
}

void tst_QAVPlayer::emptyStreams()
{
    QAVPlayer p;

    QFileInfo file(testData("guido.mp4"));

    QSignalSpy spyAudio(&p, &QAVPlayer::audioStreamsChanged);
    QSignalSpy spyVideo(&p, &QAVPlayer::videoStreamsChanged);

    QAVAudioFrame frameAudio;
    QAVVideoFrame frameVideo;
    QSet<int> streamsAudio;
    QSet<int> streamsVideo;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frameAudio = f; streamsAudio.insert(f.stream().index()); }, Qt::DirectConnection);
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frameVideo = f; streamsVideo.insert(f.stream().index()); });

    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    p.setAudioStreams({});
    p.setVideoStreams({});
    p.play();

    QTRY_COMPARE(spyAudio.count(), 1);
    QTRY_COMPARE(spyVideo.count(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);

    streamsAudio.clear();
    streamsVideo.clear();
    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QCOMPARE(streamsAudio.size(), 0);
    QCOMPARE(streamsVideo.size(), 0);

    p.setAudioStream(p.availableAudioStreams().first());
    frameAudio = QAVAudioFrame();
    p.play();

    QTRY_VERIFY(frameAudio.pts() > 0);
    QCOMPARE(streamsAudio.size(), 1);
    QVERIFY(streamsAudio.contains(p.availableAudioStreams().first().index()));
    QCOMPARE(streamsVideo.size(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QTRY_COMPARE_WITH_TIMEOUT(p.state(), QAVPlayer::StoppedState, 15000);

    p.setAudioStreams({});
    p.setVideoStreams(p.availableVideoStreams());
    frameAudio = QAVAudioFrame();
    frameVideo = QAVVideoFrame();
    streamsAudio.clear();
    streamsVideo.clear();
    p.play();

    QTRY_VERIFY(frameVideo.pts() > 0);
    QCOMPARE(streamsVideo.size(), 1);
    QVERIFY(streamsVideo.contains(p.availableVideoStreams().first().index()));
}

void tst_QAVPlayer::flushCodecs()
{
    QAVPlayer p;
    QFileInfo file(testData("DHC0413_CreaseOrNot.mp4"));
    int framesCount = 0;
    QAVFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });
    qint64 pos = 0;
    QObject::connect(&p, &QAVPlayer::played, &p, [&](qint64 p) { pos = p; });

    p.setSource(file.absoluteFilePath());
    p.setSynced(false);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QVERIFY(frame);
    QVERIFY(frame.stream());
    QCOMPARE(frame.stream().framesCount(), 309);
    if (pos > 0) {
        qDebug() << "Played from" << pos;
        return;
    }
    QTRY_COMPARE(framesCount, 309);

    frame = {};
    framesCount = 0;
    p.setSynced(true);
    QVERIFY(p.isSynced());
    p.setSpeed(2);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QVERIFY(frame);
    QVERIFY(frame.stream());
    QCOMPARE(frame.stream().framesCount(), 309);
    QTRY_COMPARE(framesCount, 309);
}
