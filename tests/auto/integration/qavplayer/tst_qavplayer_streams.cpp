/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"

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

#ifdef QT_AVPLAYER_MULTIMEDIA
void tst_QAVPlayer::cast2QVideoFrame_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QSize>("size");

    QTest::newRow("colors.mp4") << testData("colors.mp4") << QSize(160, 120);
    QTest::newRow("dv_dsf_1_stype_1.dv") << testData("dv_dsf_1_stype_1.dv") << QSize(720, 576);
    //QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << QSize(720, 576); -- "yuv411p" is not supported
    QTest::newRow("small.mp4") << testData("small.mp4") << QSize(560, 320);
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov") << QSize(1920, 1080);
}

void tst_QAVPlayer::cast2QVideoFrame()
{
    QFETCH(QString, path);
    QFETCH(QSize, size);

    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(file.absoluteFilePath());

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.pause();
    QTRY_VERIFY(frame);

    QVideoFrame q = frame;
    QVERIFY(q.isValid());
    QVERIFY(!q.size().isEmpty());
    QCOMPARE(q.size(), size);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    q.map(QAbstractVideoBuffer::ReadOnly);
    QVERIFY(q.bits() != nullptr);
    QVERIFY(q.bytesPerLine() > 0);
#else
    q.map(QVideoFrame::ReadOnly);
    QVERIFY(q.bits(0) != nullptr);
    QVERIFY(q.bytesPerLine(0) > 0);
#endif
    q = frame.toQVideoFrame();
    QVERIFY(q.isValid());
    QVERIFY(!q.size().isEmpty());
    QCOMPARE(q.size(), size);
}

void tst_QAVPlayer::audioOutput()
{
    QFileInfo file1(testData("guido.mp4"));
    QFileInfo file2(testData("small.mp4"));

    QAVAudioOutput out;
    QAVAudioFrame frame;
    QAVPlayer p;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&out, &frame](const QAVAudioFrame &f) {
        out.play(f);
        frame = f;
    });

    auto outWithParent = new QAVAudioOutput(&p);
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&outWithParent](const QAVAudioFrame &f) {
        outWithParent->play(f);
    }, Qt::DirectConnection);

    p.setSource(file1.absoluteFilePath());
    p.play();
    QTest::qWait(100);
    out.setVolume(0.9);
    QCOMPARE(out.volume(), 0.9);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    auto audioDevices = QMediaDevices::audioOutputs();
#else
    auto audioDevices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
#endif
    if (!audioDevices.isEmpty()) {
        out.setAudioDevice(audioDevices.first());
        QCOMPARE(out.audioDevice(), audioDevices.first());
    }
    out.setAudioDevice({});
    QVERIFY(out.audioDevice().isNull());

#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
    out.setChannelConfig(QAudioFormat::channelConfig(QAudioFormat::FrontRight));
    QCOMPARE(out.channelConfig(), QAudioFormat::channelConfig(QAudioFormat::FrontRight));
#endif
    p.setSource(file2.absoluteFilePath());
    p.play();
    QTRY_VERIFY(p.position() > 500);
    auto fmt = frame.format();
    auto af = QAVAudioFrame(fmt, frame.data());
    QCOMPARE(af.format(), fmt);
    QCOMPARE(af.data(), frame.data());
}

void tst_QAVPlayer::multiPlayers()
{
    QFileInfo file(testData("av_sample.mkv"));
    QAVAudioOutput o1;
    QAVAudioOutput o2;
    QAVPlayer p1;
    QAVPlayer p2;
    p1.setSource(file.absoluteFilePath());
    p2.setSource(file.absoluteFilePath());
    p1.setSynced(false);
    p2.setSynced(false);
    o1.setVolume(0);
    o2.setVolume(0);
    int framesCount1 = 0;
    int framesCount2 = 0;
    qint64 pos1 = 0;
    qint64 pos2 = 0;
    QObject::connect(&p1, &QAVPlayer::videoFrame, &p1, [&](const QAVVideoFrame &) { ++framesCount1; pos1 = p1.position(); }, Qt::DirectConnection);
    QObject::connect(&p1, &QAVPlayer::audioFrame, &p1, [&](const QAVAudioFrame &f) { o1.play(f); }, Qt::DirectConnection);
    QObject::connect(&p2, &QAVPlayer::videoFrame, &p2, [&](const QAVVideoFrame &) { ++framesCount2; pos2 = p2.position(); }, Qt::DirectConnection);
    QObject::connect(&p2, &QAVPlayer::audioFrame, &p2, [&](const QAVAudioFrame &f) { o2.play(f); }, Qt::DirectConnection);
    p1.play();
    p2.play();
    QTRY_COMPARE(p1.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p2.mediaStatus(), QAVPlayer::EndOfMedia);
    QVERIFY(pos1 > 0);
    QVERIFY(pos2 > 0);
    QVERIFY(framesCount1 > 0);
    QVERIFY(framesCount2 > 0);
}

#endif // #ifndef QT_AVPLAYER_MULTIMEDIA

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

