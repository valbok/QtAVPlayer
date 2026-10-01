/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"

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

void tst_QAVPlayer::files_io_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("duration");
    QTest::addColumn<int>("videoFrames");
    QTest::addColumn<int>("audioFrames");

    QTest::newRow("test.wav") << testData("test.wav") << 999 << 0 << 21;
    QTest::newRow("colors.mp4") << testData("colors.mp4") << 15019 << 374 << 702;
    QTest::newRow("shots0000.dv") << testData("shots0000.dv") << 40 << 1 << 0;
    QTest::newRow("dv_dsf_1_stype_1.dv") << testData("dv_dsf_1_stype_1.dv") << 600 << 14 << 14;
    QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << 2000 << 49 << 49;
    QTest::newRow("small.mp4") << testData("small.mp4") << 5568 << 165 << 259;
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov") << 6840 << 169 << 0;
}

void tst_QAVPlayer::files_io()
{
    QFETCH(QString, path);
    QFETCH(int, duration);
    QFETCH(int, videoFrames);
    QFETCH(int, audioFrames);
    const bool hasVideo = videoFrames > 0;
    const bool hasAudio = audioFrames > 0;

    QAVPlayer p;

    QFileInfo fileInfo(path);
    QSharedPointer<QIODevice> file(new QFile(fileInfo.absoluteFilePath()));
    if (!file->open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }
    QSharedPointer<QAVIODevice> dev(new QAVIODevice(file));
    p.setSource(path, dev);

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
}

