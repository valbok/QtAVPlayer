/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"
#include "qavaudiooutput.h"

#include <QtTest/QtTest>

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
#endif
