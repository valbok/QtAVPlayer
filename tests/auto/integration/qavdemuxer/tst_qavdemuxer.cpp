/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavaudioframe.h"
#include "qavvideoframe.h"
#ifndef QTAVPLAYERLIB
#include "private/qavdemuxer_p.h"
#include "private/qavvideocodec_p.h"
#include "private/qavaudiocodec_p.h"
#else
#include "qavdemuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#endif
#include <QDebug>
#include <QtTest/QtTest>

QT_USE_NAMESPACE

class tst_QAVDemuxer : public QObject
{
    Q_OBJECT
private slots:
    void construction();
    void loadIncorrect();
    void loadAudio();
    void loadVideo();
};

void tst_QAVDemuxer::construction()
{
    QAVDemuxer d;
    QVERIFY(d.videoStream() < 0);
    QVERIFY(d.audioStream() < 0);
    QVERIFY(d.subtitleStream() < 0);
    QCOMPARE(d.videoCodec(), nullptr);
    QCOMPARE(d.audioCodec(), nullptr);
    QCOMPARE(d.duration(), 0);
    QCOMPARE(d.seekable(), false);
    QCOMPARE(d.eof(), false);
    QVERIFY(!d.read());
    QVERIFY(d.seek(0) < 0);

    QAVPacket p;
    QVERIFY(!p);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.bytes(), 0);
    QVERIFY(p.streamIndex() < 0);
    QVERIFY(!p.decode());

    QAVFrame f;
    QVERIFY(!f);
    QCOMPARE(f.pts(), -1);

    QAVAudioFrame af;
    QVERIFY(!af);
    QCOMPARE(af.pts(), -1);

    QAVVideoFrame vf;
    QVERIFY(!vf);
}

void tst_QAVDemuxer::loadIncorrect()
{
    QAVDemuxer d;
    QVERIFY(d.load(QUrl("unknown.mp4")) < 0);
    QVERIFY(d.videoStream() < 0);
    QVERIFY(d.audioStream() < 0);
    QVERIFY(d.subtitleStream() < 0);
    QVERIFY(!d.read());
    QVERIFY(d.seek(0) < 0);
}

void tst_QAVDemuxer::loadAudio()
{
    QAVDemuxer d;

    auto dirName = QFileInfo(__FILE__).absoluteDir().path();
    QFileInfo file(dirName + "/../testdata/test.wav");

    QVERIFY(d.load(QUrl::fromLocalFile(file.absoluteFilePath())) >= 0);
    QVERIFY(d.videoStream() < 0);
    QVERIFY(d.audioStream() >= 0);
    QVERIFY(d.subtitleStream() < 0);
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    QAVPacket p;
    QAVPacket p2;
    QAVFrame f2;
    while ((p = d.read())) {
        QVERIFY(p);
        QVERIFY(p.packet());
        QVERIFY(p.duration() > 0);
        QVERIFY(p.pts() >= 0);
        QVERIFY(p.bytes() > 0);
        QCOMPARE(p.streamIndex(), d.audioStream());

        p2 = p;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        QAVPacket p3 = p2;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.bytes(), p.bytes());
        QCOMPARE(p2.streamIndex(), p.streamIndex());

        auto f = p.decode();
        QVERIFY(f);
        QVERIFY(f.frame());
        QVERIFY(f.pts() >= 0);

        f2 = f;
        QVERIFY(f2);
        QVERIFY(f2.frame());
        QCOMPARE(f2.pts(), f.pts());

        QAVFrame f3 = f2;
        QVERIFY(f3);
        QVERIFY(f3.frame());
        QCOMPARE(f3.pts(), f.pts());

        QAVAudioFrame af = f;
        QVERIFY(af);
        QVERIFY(af.frame());
        QCOMPARE(af.pts(), f.pts());
        QVERIFY(af.codec());

        auto format = af.format();
        QCOMPARE(format.sampleFormat(), QAVAudioFormat::Int32);
        auto data = af.data();
        QVERIFY(!data.isEmpty());

        QCOMPARE(d.eof(), false);
    }

    QVERIFY(p2);
    QVERIFY(p2.packet());
    QVERIFY(p2.duration() > 0);
    QVERIFY(p2.bytes() > 0);
    QCOMPARE(p2.streamIndex(), d.audioStream());

    QVERIFY(f2);
    QVERIFY(f2.frame());
    QVERIFY(f2.pts() > 0);

    QCOMPARE(d.eof(), true);
    QVERIFY(d.seek(0) >= 0);
}

void tst_QAVDemuxer::loadVideo()
{
    QAVDemuxer d;

    auto dirName = QFileInfo(__FILE__).absoluteDir().path();
    QFileInfo file(dirName + "/../testdata/colors.mp4");

    QVERIFY(d.load(QUrl::fromLocalFile(file.absoluteFilePath())) >= 0);
    QVERIFY(d.videoStream() >= 0);
    QVERIFY(d.audioStream() >= 0);
    QVERIFY(d.subtitleStream() < 0);
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    auto p = d.read();
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.bytes() > 0);
    QVERIFY(p.streamIndex() >= 0);

    auto f = p.decode();
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d.eof(), false);

    QVERIFY(d.seek(0) >= 0);
    while ((p = d.read())) {
        QCOMPARE(d.eof(), false);
        if (p.streamIndex() == d.videoStream()) {
            auto f = p.decode();
            QVERIFY(f);
            QVERIFY(f.frame());
            QVERIFY(f.pts() >= 0);

            QAVVideoFrame vf = f;
            QVERIFY(vf);
            QVERIFY(vf.frame());
            QCOMPARE(vf.pts(), f.pts());
            QVERIFY(vf.size().isValid());
        }
    }

    QCOMPARE(d.eof(), true);
}

QTEST_MAIN(tst_QAVDemuxer)
#include "tst_qavdemuxer.moc"
