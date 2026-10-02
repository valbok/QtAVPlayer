/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"
#include "qavformatcontext_p.h"

#include <QtTest/QtTest>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

void tst_QAVPlayer::availableAudioStreams()
{
    int framesCount = 0;
    QAVAudioFrame frame;
    QAVPlayer p;

    QFileInfo file(testData("guido.mp4"));
    QSignalSpy spy(&p, &QAVPlayer::audioStreamsChanged);
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frame = f; ++framesCount; }, Qt::DirectConnection);

    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.availableVideoStreams().size(), 1);
    QCOMPARE(p.availableAudioStreams().size(), 2);
    QCOMPARE(p.availableAudioStreams()[0].index(), 1);
    QCOMPARE(p.availableAudioStreams()[1].index(), 2);
    QCOMPARE(p.currentVideoStreams().first().index(), 0);
    QCOMPARE(p.currentAudioStreams().first().index(), 1);

    spy.clear();

    p.setAudioStream({ -1 });
    QCOMPARE(p.currentAudioStreams().first().index(), 1);
    p.setVideoStream({ -1 });
    QCOMPARE(p.currentVideoStreams().first().index(), 0);
    QCOMPARE(spy.count(), 0);

    p.setAudioStream({ 3 });
    QCOMPARE(p.currentAudioStreams().first().index(), 1);
    QCOMPARE(spy.count(), 0);

    p.setAudioStream({ 2 });
    QCOMPARE(p.currentAudioStreams().first().index(), 2);
    QTRY_COMPARE(spy.count(), 1);

    p.pause();
    QCOMPARE(p.currentAudioStreams().first().index(), 2);

    p.play();
    QTRY_VERIFY(frame);
    QVERIFY(framesCount > 0);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QCOMPARE(p.currentAudioStreams().first().index(), 2);

    framesCount = 0;

    p.play();
    QTRY_VERIFY(framesCount > 3);

    framesCount = 0;
    spy.clear();

    p.setAudioStream({ 1 });
    QTRY_COMPARE(spy.count(), 1);
    QTRY_VERIFY(framesCount > 3);
    QCOMPARE(p.currentAudioStreams().size(), 1);
    QCOMPARE(p.currentAudioStreams().first().index(), 1);

    framesCount = 0;

    p.stop();
    p.seek(20);
    p.pause();
    p.play();
    QTRY_VERIFY(framesCount > 3);
    QCOMPARE(p.currentAudioStreams().first().index(), 1);
}

void tst_QAVPlayer::subtitles()
{
    QAVPlayer p;

    QFileInfo file(testData("colors_subtitles.mp4"));
    p.setSource(file.absoluteFilePath());

    QSignalSpy spy(&p, &QAVPlayer::subtitleStreamsChanged);

    QAVSubtitleFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::subtitleFrame, &p, [&](const QAVSubtitleFrame &f) { frame = f; ++framesCount; });

    p.play();

    QTRY_VERIFY(!p.availableSubtitleStreams().isEmpty());
    QCOMPARE(p.availableSubtitleStreams().size(), 2);
    QCOMPARE(p.availableSubtitleStreams()[0].index(), 2);
    QCOMPARE(p.availableSubtitleStreams()[1].index(), 3);
    QVERIFY(!p.currentSubtitleStreams().isEmpty());
    QCOMPARE(p.currentSubtitleStreams().size(), 1);
    QCOMPARE(p.currentSubtitleStreams().first().index(), 2);
    QVERIFY(p.currentSubtitleStreams().first().stream() != nullptr);
    QCOMPARE(p.currentSubtitleStreams().first().duration(), 45.809);
    QVERIFY(!p.currentSubtitleStreams().first().metadata().isEmpty());
    QCOMPARE(p.currentSubtitleStreams().first().metadata()["language"], QStringLiteral("eng"));
    QCOMPARE(p.currentSubtitleStreams().first().framesCount(), 9);
    QTRY_VERIFY(frame);
    QVERIFY(frame.subtitle() != nullptr);
    QCOMPARE(frame.subtitle()->num_rects, 1u);
    QCOMPARE(spy.count(), 0);
    QTRY_VERIFY_WITH_TIMEOUT(framesCount > 3, 20000);

    frame = QAVSubtitleFrame();

    p.seek(0);
    p.setSpeed(3);
    p.setSubtitleStream({3});

    QCOMPARE(p.currentSubtitleStreams().first().index(), 3);
    QVERIFY(p.currentSubtitleStreams().first().stream() != nullptr);
    QCOMPARE(p.currentSubtitleStreams().first().duration(), 45.809);
    QVERIFY(!p.currentSubtitleStreams().first().metadata().isEmpty());
    QCOMPARE(p.currentSubtitleStreams().first().metadata()["language"], QStringLiteral("nor"));

    p.play();

    QTRY_VERIFY(frame);
    QTRY_COMPARE(spy.count(), 1);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 20000);
    QVERIFY(frame.subtitle() != nullptr);
    QVERIFY(frame.subtitle()->rects != nullptr);
}

void tst_QAVPlayer::multipleAudioStreams()
{
    QSet<int> streams;
    QAVPlayer p;

    QFileInfo file(testData("guido.mp4"));

    QSignalSpy spy(&p, &QAVPlayer::audioStreamsChanged);
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&streams](const QAVAudioFrame &f) { streams.insert(f.stream().index()); }, Qt::DirectConnection);

    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    auto audioStreams = p.availableAudioStreams();
    auto videoStreams = p.availableVideoStreams();
    QCOMPARE(audioStreams.size(), 2);
    QCOMPARE(audioStreams[0].duration(), 3);
    QCOMPARE(audioStreams[0].framesCount(), 125);
    QCOMPARE(audioStreams[1].duration(), 3);
    QCOMPARE(audioStreams[1].framesCount(), 125);
    QCOMPARE(videoStreams.size(), 1);
    QCOMPARE(videoStreams[0].duration(), 2.3773788);
    QCOMPARE(videoStreams[0].framesCount(), 57);
    p.setAudioStreams(p.availableAudioStreams());
    p.setAudioStreams(p.availableAudioStreams());
    QTRY_COMPARE(spy.count(), 1);
    p.play();

    QTRY_COMPARE(streams.size(), p.availableAudioStreams().size());
    for (const auto &stream: p.availableAudioStreams())
        QVERIFY(streams.contains(stream.index()));
    QCOMPARE(spy.count(), 1);
}

void tst_QAVPlayer::multipleVideoStreams_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("streamsCount");
    QTest::addColumn<QList<double>>("streamsDurations");
    QTest::addColumn<QList<int>>("streamsFramesCount");

    QTest::newRow("7_BCL02006_ffv1_20s_1.mkv") << "7_BCL02006_ffv1_20s_1.mkv" << 4 << QList<double>{20, 20.025, 20.025, 20.025} << QList<int>{599, 2, 2, 2};
    QTest::newRow("7_BCL02006_ffv1_20s_2.mkv") << "7_BCL02006_ffv1_20s_2.mkv" << 4 << QList<double>{20, 20.025, 20.025, 20.025} << QList<int>{599, 1, 2, 2};
}

void tst_QAVPlayer::multipleVideoStreams()
{
    QFETCH(QString, path);
    QFETCH(int, streamsCount);
    QFETCH(QList<double>, streamsDurations);
    QFETCH(QList<int>, streamsFramesCount);

    QAVPlayer p;
    QFileInfo file(testData(path));

    QMap<int, int> framesCount;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { framesCount[f.stream().index()]++; });

    p.setSource(file.absoluteFilePath());
    p.setSynced(false);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    auto audioStreams = p.availableAudioStreams();
    auto videoStreams = p.availableVideoStreams();
    QCOMPARE(audioStreams.size(), 0);
    QCOMPARE(videoStreams.size(), streamsCount);
    for (int i = 0; i < streamsCount; ++i)
        QCOMPARE(videoStreams[i].duration(), streamsDurations[i]);
    // Set all video streams
    p.setVideoStreams(p.availableVideoStreams());
    p.play();

    QTRY_COMPARE(framesCount.size(), p.availableVideoStreams().size());
    for (int i = 0; i < streamsCount; ++i)
        QTRY_COMPARE(framesCount[i], streamsFramesCount[i]);
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

void tst_QAVPlayer::streamMetadataRotate()
{
    QAVPlayer p;
    QFileInfo file(testData("rotated_90.mp4"));
    p.setSource(file.absoluteFilePath());
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.currentVideoStreams().size(), 1);
    QVERIFY(!p.currentVideoStreams()[0].metadata().isEmpty());
    QVERIFY(p.currentVideoStreams()[0].metadata().contains("rotate"));
    QCOMPARE(p.currentVideoStreams()[0].metadata()["rotate"], "90");
}

void tst_QAVPlayer::klvStreamInfo()
{
    auto ctx = QAVFormatContext::alloc();
    QVERIFY(ctx);
    auto avStream = avformat_new_stream(ctx->ctx(), nullptr);
    QVERIFY(avStream);
    avStream->codecpar->codec_type = AVMEDIA_TYPE_DATA;
    avStream->codecpar->codec_id = AV_CODEC_ID_SMPTE_KLV;

    QAVStream stream(0, ctx);
    const auto info = stream.info();
    QCOMPARE(info.mediaType, QStringLiteral("data"));
    QCOMPARE(info.codecName, QStringLiteral("klv"));
}
