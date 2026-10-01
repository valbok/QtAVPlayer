/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"
#include "qavmuxerframes.h"

#include <QtTest/QtTest>

void tst_QAVPlayer::outputFile()
{
    QAVPlayer p;
    QList<QString> files = {"av_sample.mkv", "small.mp4"};
    p.setSynced(false);
    for (const auto &f : files) {
        QFileInfo file(testData(f));
        p.setSource(file.absoluteFilePath());
        p.setOutput("output.mkv");
        QCOMPARE(p.output(), "output.mkv");
        p.play();
        QTRY_VERIFY(p.mediaStatus() == QAVPlayer::EndOfMedia);
    }

    p.setSource(QLatin1String("unknown.mp4"), nullptr);
    p.setOutput("output.mkv");
    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);

    p.setOutput("");
    p.setSource(QFileInfo(testData("av_sample.mkv")).absoluteFilePath());
    p.play();
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia || p.mediaStatus() == QAVPlayer::EndOfMedia);

    QAVPlayer::Error err;
    QObject::connect(&p, &QAVPlayer::errorOccurred, &p, [&](auto error, auto) { err = error; });
    p.setSource(QFileInfo(testData("rotated_90.mp4")).absoluteFilePath());
    p.setOutput("output");
    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QTRY_COMPARE(err, QAVPlayer::MuxerError);

    // Set before the source
    p.setOutput({});
    p.setSource(QFileInfo(testData("av_sample.mkv")).absoluteFilePath());
    p.play();
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia || p.mediaStatus() == QAVPlayer::EndOfMedia);

    p.setSource(QFileInfo(testData("av_sample.mkv")).absoluteFilePath());
    p.play();
    p.setOutput("output.mkv");
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia || p.mediaStatus() == QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::muxerFilters()
{
    QAVPlayer p;
    QAVMuxerFrames m;
    p.setSynced(false);
    p.setInputVideoCodec("software");
    p.setFilter("curves=vintage");
    p.setSource(QFileInfo(testData("small.mp4")).absoluteFilePath());

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        QVERIFY(m.write(f) < 0);
        auto frame = f.convertTo(AV_PIX_FMT_YUV420P);
        QVERIFY(m.write(frame) == 0);
    }, Qt::DirectConnection);
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, &p, [&](auto status) {
        if (status == QAVPlayer::LoadedMedia) {
            auto streams = p.availableVideoStreams();
            QCOMPARE(streams.size(), 1);
            QVERIFY(m.load(streams, "output.mkv") == 0);
            p.play();
        }
    });
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::EndOfMedia);
    m.unload();
    QVERIFY(m.load(p.availableStreams(), "output.mkv") == 0);
}

void tst_QAVPlayer::muxerMultiSourceFrames()
{
    QAVPlayer p1;
    QAVPlayer p2;
    QAVMuxerFrames m;
    p1.setSynced(false);
    p1.setInputVideoCodec("software");
    p1.setSource(QFileInfo(testData("small.mp4")).absoluteFilePath());
    p2.setSynced(false);
    p2.setInputVideoCodec("software");
    p2.setSource(QFileInfo(testData("av_sample.mkv")).absoluteFilePath());

    QObject::connect(&p1, &QAVPlayer::videoFrame, &p1, [&](const QAVVideoFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
    QObject::connect(&p1, &QAVPlayer::audioFrame, &p1, [&](const QAVAudioFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
    QObject::connect(&p2, &QAVPlayer::videoFrame, &p2, [&](const QAVVideoFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
    QObject::connect(&p2, &QAVPlayer::audioFrame, &p2, [&](const QAVAudioFrame &f) { m.enqueue(f); }, Qt::DirectConnection);

    QTRY_VERIFY(p1.mediaStatus() == QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p2.mediaStatus() == QAVPlayer::LoadedMedia);
    auto streams = p1.availableStreams() + p2.availableStreams();
    QVERIFY(m.load(streams, "output.mkv") == 0);
    p1.play();
    p2.play();
    QTRY_VERIFY(p1.mediaStatus() == QAVPlayer::EndOfMedia);
    QTRY_VERIFY(p2.mediaStatus() == QAVPlayer::EndOfMedia);
    m.unload();
    QAVPlayer p;
    p.setSource("output.mkv");
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia);
    QVERIFY(!p.availableStreams().isEmpty());
    QCOMPARE(p.availableStreams().size(), 4);
}

void tst_QAVPlayer::scaleHW()
{
    QAVPlayer p;
    p.setSynced(false);
    QSize size;
#if defined(QT_AVPLAYER_CUDA)
    p.setInputVideoCodec("h264_cuvid");
    p.setFilter("scale_cuda=1920:1080");
    size = {1920, 1080};
#endif
#if defined(QT_AVPLAYER_VULKAN)
    p.setInputVideoCodec("");
    p.setFilter("scale_vulkan=1920:1080");
    size = {1920, 1080};
#endif
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    p.setFilter("scale_vt=1920:1080");
    //size = {1920, 1080}; // TODO: ci could fail to initialize videotoolbox_vld
#endif
#if defined(Q_OS_WIN)
    p.setFilter("scale_d3d11=1920:1080");
    //size = {1920, 1080};
#endif
    if (size.isEmpty())
        return;
    p.setSource(QFileInfo(testData("small.mp4")).absoluteFilePath());
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        QCOMPARE(f.size(), size);
    }, Qt::DirectConnection);

    p.play();
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::muxerScaleHW_data()
{
    QTest::addColumn<QString>("decoder");
    QTest::addColumn<QString>("encoder");
    QTest::addColumn<QString>("filter");
#if defined(QT_AVPLAYER_CUDA)
    QTest::newRow("cuda") << "h264_cuvid" << "h264_nvenc" << "scale_cuda";
#endif
#if defined(QT_AVPLAYER_VULKAN)
    QTest::newRow("vulkan") << "" << "h264_vulkan" << "scale_vulkan";
#endif
}

void tst_QAVPlayer::muxerScaleHW()
{
    if (!QTest::currentDataTag())
        QSKIP("No data");
    QFETCH(QString, decoder);
    QFETCH(QString, encoder);
    QFETCH(QString, filter);

    QAVMuxerFrames m;
    QAVPlayer p;
    p.setSynced(false);
    QSize size(160, 120);
    p.setInputVideoCodec(decoder);
    p.setFilter(filter + "=160:120");
    p.setSource(QFileInfo(testData("small.mp4")).absoluteFilePath());

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        QCOMPARE(f.size(), size);
        m.enqueue(f);
    }, Qt::DirectConnection);
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) {
        m.enqueue(f);
    }, Qt::DirectConnection);

    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia);
    auto videoStreams = p.availableVideoStreams();
    QCOMPARE(videoStreams.size(), 1);
    auto c = videoStreams[0].codec();
    QVERIFY(c);
    QCOMPARE(c->size(), QSize(560, 320));
    QList<QAVMuxerFrames::EncoderStream> encoderStreams;
    for (auto &s : videoStreams)
        encoderStreams.push_back({s, encoder, size});
    for (auto &s : p.availableAudioStreams())
        encoderStreams.push_back(s);
    // Make sure that AVCodecContext::get_format() is called
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);
    p.pause();
    QTRY_COMPARE(spyPaused.count(), 1);
    QVERIFY(m.load(encoderStreams, "output.mkv") >= 0);

    p.play();
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::EndOfMedia);
    QTRY_VERIFY(m.size() == 0);
    QVERIFY(m.flush() >= 0);
    m.unload();

    QAVPlayer p2;
    QAVVideoFrame vf;
    QObject::connect(&p2, &QAVPlayer::videoFrame, &p2, [&](const QAVVideoFrame &f) {
        vf = f;
    });

    p2.setSource("output.mkv");
    QTRY_VERIFY(p2.mediaStatus() == QAVPlayer::LoadedMedia);
    p2.pause();
    QTRY_VERIFY(vf);
    QCOMPARE(vf.size(), size);
}

void tst_QAVPlayer::muxerScaleHWSplit()
{
    QAVMuxerFrames m;
    QAVPlayer p;
    p.setSynced(false);
    QSize size;
    QString codec;
#if defined(QT_AVPLAYER_CUDA)
    p.setInputVideoCodec("h264_cuvid");
    p.setFilter("[0:v]split=2[orig][toscale];[toscale]scale_cuda=160:120[scaled]");
    size = {160, 120};
    codec = "h264_nvenc";
#endif
    if (size.isEmpty())
        return;
    p.setSource(QFileInfo(testData("small.mp4")).absoluteFilePath());

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        if (f.filterName() == QLatin1String("orig")) {
            QCOMPARE(f.size(), QSize(560, 320));
            return;
        }
        QCOMPARE(f.filterName(), QLatin1String("scaled"));
        QCOMPARE(f.size(), size);
        m.enqueue(f);
    }, Qt::DirectConnection);

    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia);
    auto videoStreams = p.availableVideoStreams();
    QCOMPARE(videoStreams.size(), 1);
    auto c = videoStreams[0].codec();
    QVERIFY(c);
    QCOMPARE(c->size(), QSize(560, 320));
    QVERIFY(m.load({{videoStreams[0], codec, size}}, "output.mkv") >= 0);

    p.play();
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::EndOfMedia);
    QTRY_VERIFY(m.size() == 0);
    QVERIFY(m.flush() >= 0);
    m.unload();

    QAVPlayer p2;
    QAVVideoFrame vf;
    QObject::connect(&p2, &QAVPlayer::videoFrame, &p2, [&](const QAVVideoFrame &f) {
        vf = f;
    });

    p2.setSource("output.mkv");
    QTRY_VERIFY(p2.mediaStatus() == QAVPlayer::LoadedMedia);
    p2.pause();
    QTRY_VERIFY(vf);
    QCOMPARE(vf.size(), size);
}

void tst_QAVPlayer::muxerScale_data()
{
    QTest::addColumn<QString>("decoder");
    QTest::addColumn<QString>("encoder");
    QTest::newRow("software") << "software" << "";
#if defined(QT_AVPLAYER_CUDA)
    QTest::newRow("cuda") << "h264_cuvid" << "h264_nvenc";
#endif
#if defined(QT_AVPLAYER_VULKAN)
    QTest::newRow("vulkan") << "" << "h264_vulkan";
#endif
}

void tst_QAVPlayer::muxerScale()
{
    QFETCH(QString, decoder);
    QFETCH(QString, encoder);
    QAVMuxerFrames m;
    QAVPlayer p;
    p.setSynced(false);
    QSize size(160, 120);

    p.setInputVideoCodec(decoder);
    p.setSource(QFileInfo(testData("small.mp4")).absoluteFilePath());

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        m.enqueue(f);
    }, Qt::DirectConnection);

    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::LoadedMedia);
    auto videoStreams = p.availableVideoStreams();
    QCOMPARE(videoStreams.size(), 1);
    auto c = videoStreams[0].codec();
    QVERIFY(c);
    QCOMPARE(c->size(), QSize(560, 320));
    // Make sure that AVCodecContext::get_format() is called
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);
    p.pause();
    QTRY_COMPARE(spyPaused.count(), 1);
    QVERIFY(m.load({{videoStreams[0], encoder, size}}, "output.mkv") >= 0);

    p.play();
    QTRY_VERIFY(p.mediaStatus() == QAVPlayer::EndOfMedia);
    QTRY_VERIFY(m.size() == 0);
    QVERIFY(m.flush() >= 0);
    m.unload();

    QAVPlayer p2;
    QAVVideoFrame vf;
    QObject::connect(&p2, &QAVPlayer::videoFrame, &p2, [&](const QAVVideoFrame &f) {
        vf = f;
    }, Qt::QueuedConnection);

    p2.setSource("output.mkv");
    QTRY_VERIFY(p2.mediaStatus() == QAVPlayer::LoadedMedia);
    p2.pause();
    QTRY_VERIFY(vf);
    QCOMPARE(vf.size(), size);
}
