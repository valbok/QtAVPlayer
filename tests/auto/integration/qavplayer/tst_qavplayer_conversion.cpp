/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"

#include <QtTest/QtTest>

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
