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
#include <QThreadPool>

void tst_QAVPlayer::initTestCase()
{
    QThreadPool::globalInstance()->setMaxThreadCount(20);
}

void tst_QAVPlayer::construction()
{
    QAVPlayer p;
    QVERIFY(p.source().isEmpty());
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(p.availableSubtitleStreams().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.speed(), 1.0);
    QVERIFY(!p.isSeekable());
}

void tst_QAVPlayer::sourceChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::sourceChanged);
    p.setSource(QLatin1String("unknown.mp4"));
    QCOMPARE(spy.count(), 1);
    p.setSource(QLatin1String("unknown.mp4"));
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
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 0);
    QCOMPARE(p.videoFrameRate(), 0.0);

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 1);
    QCOMPARE(p.videoFrameRate(), 0.04);
    QVERIFY(qAbs(p.duration() - 15019) < 2);
    QVERIFY(!p.availableAudioStreams().isEmpty());
    QVERIFY(!p.availableVideoStreams().isEmpty());
    QCOMPARE(p.currentVideoStreams().first().framesCount(), 375);
    QCOMPARE(p.currentVideoStreams().first().frameRate(), 0.04);
    QCOMPARE(p.currentAudioStreams().first().framesCount(), 704);
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

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
}

void tst_QAVPlayer::speedVideo()
{
    QAVPlayer p;

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.setSpeed(5);
    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);

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

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.play();
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.stop();
}

void tst_QAVPlayer::files_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("duration");
    QTest::addColumn<bool>("hasVideo");
    QTest::addColumn<bool>("hasAudio");

    QTest::newRow("test.wav") << testData("test.wav") << 999 << false << true;
    QTest::newRow("colors.mp4") << testData("colors.mp4") << 15019 << true << true;
    QTest::newRow("shots0000.dv") << testData("shots0000.dv") << 40 << true << false;
    QTest::newRow("dv_dsf_1_stype_1.dv") << testData("dv_dsf_1_stype_1.dv") << 600 << true << true;
    QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << 2000 << true << true;
    QTest::newRow("small.mp4") << testData("small.mp4") << 5568 << true << true;
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov") << 6840 << true << false;
    QTest::newRow("star_trails.mpeg") << testData("star_trails.mpeg") << 1050 << true << true;
}

void tst_QAVPlayer::files()
{
    QFETCH(QString, path);
    QFETCH(int, duration);
    QFETCH(bool, hasVideo);
    QFETCH(bool, hasAudio);

    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(file.absoluteFilePath());

    int vf = 0;
    QAVVideoFrame videoFrame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { videoFrame = f; if (f) ++vf; });
    int af = 0;
    QAVAudioFrame audioFrame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { audioFrame = f; if (f) ++af; });

    p.pause();
    if (hasVideo) {
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || videoFrame);
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || vf == 1);
    }
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia || p.mediaStatus() == QAVPlayer::EndOfMedia);
    QTRY_VERIFY(qAbs(p.duration() - duration) < 2);
    QCOMPARE(!p.availableVideoStreams().isEmpty(), hasVideo);
    QCOMPARE(!p.availableAudioStreams().isEmpty(), hasAudio);

    af = 0;
    vf = 0;
    videoFrame = QAVVideoFrame();

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

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 18000);

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
    bool eof = p.mediaStatus() == QAVPlayer::EndOfMedia;
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, &p, [&](QAVPlayer::MediaStatus s) { if (!eof) eof = s == QAVPlayer::EndOfMedia; });
    p.play();
    if (hasVideo)
        QTRY_VERIFY(p.state() == QAVPlayer::StoppedState || videoFrame);

    p.play();
    p.stop();
    QTest::qWait(100);

    p.pause();
    p.stop();
    p.pause();
    p.play();
    p.seek(duration * 0.9);
    QTRY_VERIFY(eof);
    for (const auto &s : p.availableVideoStreams()) {
        auto progress = p.progress(s);
        QVERIFY(progress.pts() >= 0.0);
        QVERIFY(progress.framesCount() > 0);
        QVERIFY(progress.frameRate() > 0.0);
        QVERIFY(progress.fps() > 0);
    }
}

void tst_QAVPlayer::convert_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<AVPixelFormat>("to");

    QTest::newRow("colors.mp4") << testData("colors.mp4") << AV_PIX_FMT_NV12;
    QTest::newRow("dv_dsf_1_stype_1.dv") << testData("dv_dsf_1_stype_1.dv") << AV_PIX_FMT_NV21;
    QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << AV_PIX_FMT_YUV420P;
    QTest::newRow("small.mp4") << testData("small.mp4") << AV_PIX_FMT_YUV422P;
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov") << AV_PIX_FMT_NV12;
    QTest::newRow("1.dv") << testData("1.dv") << AV_PIX_FMT_YUV422P;
}

void tst_QAVPlayer::convert()
{
    QFETCH(QString, path);
    QFETCH(AVPixelFormat, to);

    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(file.absoluteFilePath());

    QAVVideoFrame videoFrame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { videoFrame = f; });

    p.pause();
    QTRY_VERIFY(videoFrame);

    QAVVideoFrame converted = videoFrame.convertTo(to);
    QVERIFY(converted);
    QCOMPARE(converted.format(), to);
    QCOMPARE(converted.pts(), videoFrame.pts());
    QCOMPARE(converted.size(), videoFrame.size());

    const QSize size(128, 72);
    QAVVideoFrame convertedSize = videoFrame.convertTo(to, size);
    QVERIFY(convertedSize);
    QCOMPARE(convertedSize.format(), to);
    QCOMPARE(convertedSize.pts(), videoFrame.pts());
    QCOMPARE(convertedSize.size(), size);
}

void tst_QAVPlayer::map_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("colors.mp4") << testData("colors.mp4");
    QTest::newRow("dv_dsf_1_stype_1.dv") << testData("dv_dsf_1_stype_1.dv");
    QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv");
    QTest::newRow("small.mp4") << testData("small.mp4");
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov");
}

void tst_QAVPlayer::map()
{
    QFETCH(QString, path);

    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(file.absoluteFilePath());

    QAVVideoFrame frame;
    QVERIFY(!frame.isMapped());
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.play();
    QTRY_VERIFY(frame);

    auto mapData = frame.map();
    QVERIFY(frame.isMapped());
    QVERIFY(mapData.size > 0);
    QVERIFY(mapData.bytesPerLine[0] > 0);
    QVERIFY(mapData.bytesPerLine[1] > 0);
    QVERIFY(mapData.data[0] != nullptr);
    QVERIFY(mapData.data[1] != nullptr);
    auto f = frame;
    QVERIFY(f.isMapped());
    auto md = f.map();
    QVERIFY(md.format == mapData.format);
    QVERIFY(md.size == mapData.size);
    QVERIFY(md.bytesPerLine[0] == mapData.bytesPerLine[0]);
    QVERIFY(md.bytesPerLine[1] == mapData.bytesPerLine[1]);
    QVERIFY(md.data[0] == mapData.data[0]);
    QVERIFY(md.data[1] == mapData.data[1]);
}

void tst_QAVPlayer::setEmptySource()
{
    QAVPlayer p;

    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &) { ++framesCount; });

    bool noMediaReceived = false;
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, &p, [&](QAVPlayer::MediaStatus s) {
        if (!noMediaReceived)
            noMediaReceived = s == QAVPlayer::NoMedia;
    });

    QFileInfo file(testData("small.mp4"));
    p.setSource(file.absoluteFilePath());
    p.play();

    QTest::qWait(200);

    p.setSource("");
    QTRY_VERIFY(noMediaReceived);

    framesCount = 0;
    QTest::qWait(100);
    QCOMPARE(framesCount, 0);
}

void tst_QAVPlayer::synced()
{
    QAVPlayer p;

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QSignalSpy spy(&p, &QAVPlayer::syncedChanged);

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QVERIFY(p.isSynced());
    p.setSynced(true);

    p.play();

    QTRY_VERIFY(p.position() > 500);
    QCOMPARE(spy.count(), 0);

    p.setSynced(false);

    QVERIFY(!p.isSynced());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.position(), p.duration());
    QTRY_VERIFY(framesCount > 200);
}

void tst_QAVPlayer::convertDirectConnection()
{
    QAVPlayer p;
    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    int frameCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        QAVVideoFrame videoFrame = frame.convertTo(AVPixelFormat::AV_PIX_FMT_YUV420P);
        ++frameCount;
    }, Qt::DirectConnection);

    p.play();
    QTRY_VERIFY(frameCount > 3);
}

void tst_QAVPlayer::mapTwice()
{
    QAVPlayer p;
    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());
    QAVVideoFrame::MapData md1;
    QAVVideoFrame::MapData md2;
    QAVVideoFrame::MapData md3;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        md1 = frame.map();
        QVERIFY(frame.isMapped());
        md2 = frame.map();
        QVERIFY(frame.isMapped());
    });
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        md3 = frame.map();
        QVERIFY(frame.isMapped());
        md3 = frame.map();
        QVERIFY(frame.isMapped());
    }, Qt::DirectConnection);

    p.pause();
    QTRY_VERIFY(md1.format != AV_PIX_FMT_NONE);
    QVERIFY(md2.format != AV_PIX_FMT_NONE);
    QCOMPARE(md1.format, md2.format);
    QTRY_VERIFY(md3.format != AV_PIX_FMT_NONE);
    QVERIFY(md3.format != AV_PIX_FMT_NONE);
}

void tst_QAVPlayer::changeFormat()
{
    QAVPlayer p;
    QFileInfo file(testData("1.dv"));
    p.setFilter("[0:v]split=2[in1][in2];[in1]boxblur[out1];[in2]negate[out2]");
    p.setSource(file.absoluteFilePath());
    QAVVideoFrame videoFrame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        videoFrame = frame;
    });

    p.play();
    QTRY_VERIFY_WITH_TIMEOUT(videoFrame, 30000);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
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

void tst_QAVPlayer::switchingSource()
{
    QAVPlayer p;
    QList<QString> files = {"av_sample.mkv", "test.mkv", "small.mp4"};
    p.setSynced(false);
    for (const auto &f : files) {
        QFileInfo file(testData(f));
        p.setSource(file.absoluteFilePath());
        p.play();
        QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia || p.mediaStatus() == QAVPlayer::EndOfMedia);
    }

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::framesAfterPlayerDestroyed()
{
    QFileInfo file(testData("small.mp4"));
    auto p = std::make_unique<QAVPlayer>();

    QAVVideoFrame videoFrame;
    QObject::connect(p.get(), &QAVPlayer::videoFrame, p.get(), [&](const QAVVideoFrame &f) { videoFrame = f; });

    p->setSource(file.absoluteFilePath());
    p->pause();
    QCOMPARE(p->state(), QAVPlayer::PausedState);
    QTRY_COMPARE(p->mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(videoFrame);
    QVERIFY(videoFrame.pts() >= 0);
    p->setSource({});
    QTRY_COMPARE(p->mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(videoFrame.pts() >= 0);

    p = nullptr;
    QVERIFY(videoFrame.pts() >= 0);

    p = std::make_unique<QAVPlayer>();
    QObject::connect(p.get(), &QAVPlayer::videoFrame, p.get(), [&](const QAVVideoFrame &f) { videoFrame = f; });
    p->setSource(file.absoluteFilePath());
    p->play();
    QTRY_COMPARE(p->mediaStatus(), QAVPlayer::LoadedMedia);
    // Destroy the player but the frame could be still active
    p = nullptr;
}

QTEST_MAIN(tst_QAVPlayer)
