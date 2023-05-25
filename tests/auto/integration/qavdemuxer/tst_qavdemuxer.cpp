/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavdemuxer_p.h"
#include "qavaudioframe.h"
#include "qavvideoframe.h"
#include "qaviodevice_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"

#include <QDebug>
#include <QtTest/QtTest>

extern "C" {
#include <libavcodec/avcodec.h>
}

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "../testdata"
#endif

QT_USE_NAMESPACE

class tst_QAVDemuxer : public QObject
{
    Q_OBJECT
    QString testData(const QString &fn) { return QLatin1String(TEST_DATA_DIR) + "/" + fn; }
private slots:
    void construction();
    void loadIncorrect();
    void loadAudio();
    void loadVideo();
    void fileIO();
    void qrcIO();
    void supportedFormats();
    void metadata();
    void videoCodecs();
    void inputOptions();
};

void tst_QAVDemuxer::construction()
{
    QAVDemuxer d;
    QVERIFY(d.currentVideoStreams().isEmpty());
    QVERIFY(d.currentAudioStreams().isEmpty());
    QVERIFY(d.currentSubtitleStreams().isEmpty());
    QCOMPARE(d.duration(), 0);
    QCOMPARE(d.seekable(), false);
    QCOMPARE(d.eof(), false);
    QVERIFY(!d.read());
    QVERIFY(d.seek(0) < 0);

    QAVPacket p;
    QVERIFY(!p);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.packet()->size, 0);
    QVERIFY(p.packet()->stream_index < 0);
    QList<QAVFrame> fs;
    d.decode(p, fs);
    QVERIFY(fs.isEmpty());

    QAVFrame f;
    QVERIFY(!f);
    QVERIFY(isnan(f.pts()));

    QAVAudioFrame af;
    QVERIFY(!af);
    QVERIFY(isnan(af.pts()));

    QAVVideoFrame vf;
    QVERIFY(!vf);
}

void tst_QAVDemuxer::loadIncorrect()
{
    QAVDemuxer d;
    QVERIFY(d.load("unknown.mp4") < 0);
    QVERIFY(d.currentVideoStreams().isEmpty());
    QVERIFY(d.currentAudioStreams().isEmpty());
    QVERIFY(d.currentSubtitleStreams().isEmpty());
    QVERIFY(!d.read());
    QVERIFY(d.seek(0) < 0);
}

void tst_QAVDemuxer::loadAudio()
{
    QAVDemuxer d;

    QFileInfo file(testData("test.wav"));

    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    QVERIFY(d.currentVideoStreams().isEmpty());
    QVERIFY(!d.currentAudioStreams().isEmpty());
    QVERIFY(d.currentSubtitleStreams().isEmpty());
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
        QVERIFY(p.packet()->size > 0);
        QCOMPARE(p.packet()->stream_index, d.currentAudioStreams().first().index());

        p2 = p;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.packet()->size, p.packet()->size);
        QCOMPARE(p2.packet()->stream_index, p.packet()->stream_index);

        QAVPacket p3 = p2;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.packet()->size, p.packet()->size);
        QCOMPARE(p2.packet()->stream_index, p.packet()->stream_index);

        QList<QAVFrame> fs;
        d.decode(p, fs);
        QCOMPARE(fs.size(), 1);
        auto f = fs[0];
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
        QVERIFY(af.stream().codec()->codec() != nullptr);

        auto format = af.format();
        QCOMPARE(format.sampleFormat(), QAVAudioFormat::Int32);
        auto data = af.data();
        QVERIFY(!data.isEmpty());

        QCOMPARE(d.eof(), false);
    }

    QVERIFY(p2);
    QVERIFY(p2.packet());
    QVERIFY(p2.duration() > 0);
    QVERIFY(p2.packet()->size > 0);
    QCOMPARE(p2.packet()->stream_index, d.currentAudioStreams().first().index());

    QVERIFY(f2);
    QVERIFY(f2.frame());
    QVERIFY(f2.pts() > 0);

    QCOMPARE(d.eof(), true);
    QVERIFY(d.seek(0) >= 0);
}

void tst_QAVDemuxer::loadVideo()
{
    QAVDemuxer d;

    QFileInfo file(testData("colors.mp4"));

    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    QVERIFY(!d.currentVideoStreams().isEmpty());
    QVERIFY(!d.currentAudioStreams().isEmpty());
    QVERIFY(d.currentSubtitleStreams().isEmpty());
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    auto p = d.read();
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.packet()->size > 0);
    QVERIFY(p.packet()->stream_index >= 0);

    QList<QAVFrame> fs;
    d.decode(p, fs);
    QCOMPARE(fs.size(), 1);
    auto f = fs[0];
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d.eof(), false);

    QVERIFY(d.seek(0) >= 0);
    while ((p = d.read())) {
        QCOMPARE(d.eof(), false);
        if (p.packet()->stream_index == d.currentVideoStreams().first().index()) {
            QList<QAVFrame> fs1;
            d.decode(p, fs1);
            QCOMPARE(fs1.size(), 1);
            f = fs1[0];
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

void tst_QAVDemuxer::fileIO()
{
    QAVDemuxer d;

    QFile file(testData("colors.mp4"));
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    QAVIODevice dev(file);
    QVERIFY(d.load("colors.mp4", &dev) >= 0);

    QVERIFY(!d.currentVideoStreams().isEmpty());
    QVERIFY(!d.currentAudioStreams().isEmpty());
    QVERIFY(d.currentSubtitleStreams().isEmpty());
    QVERIFY(d.duration() > 0);
    QCOMPARE(d.seekable(), true);
    QCOMPARE(d.eof(), false);

    auto p = d.read();
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.packet()->size > 0);
    QVERIFY(p.packet()->stream_index >= 0);

    QList<QAVFrame> fs;
    d.decode(p, fs);
    QCOMPARE(fs.size(), 1);
    auto f = fs[0];
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d.eof(), false);

    QVERIFY(d.seek(0) >= 0);
    while ((p = d.read())) {
        QCOMPARE(d.eof(), false);
        if (p.packet()->stream_index == d.currentVideoStreams().first().index()) {
            QList<QAVFrame> fs1;
            d.decode(p, fs1);
            QCOMPARE(fs1.size(), 1);
            f = fs1[0];
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

void tst_QAVDemuxer::qrcIO()
{
    QAVDemuxer d;

    QFile file(":/test.wav");
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    QAVIODevice dev(file);
    QVERIFY(d.load(QLatin1String("test.wav"), &dev) >= 0);
    QVERIFY(d.currentVideoStreams().isEmpty());
    QVERIFY(!d.currentAudioStreams().isEmpty());
    QVERIFY(d.currentSubtitleStreams().isEmpty());
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
        QVERIFY(p.packet()->size > 0);
        QCOMPARE(p.packet()->stream_index, d.currentAudioStreams().first().index());

        p2 = p;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.packet()->size, p.packet()->size);
        QCOMPARE(p2.packet()->stream_index, p.packet()->stream_index);

        QAVPacket p3 = p2;
        QVERIFY(p2);
        QCOMPARE(p2.duration(), p.duration());
        QCOMPARE(p2.packet()->size, p.packet()->size);
        QCOMPARE(p2.packet()->stream_index, p.packet()->stream_index);

        QList<QAVFrame> fs;
        d.decode(p, fs);
        QCOMPARE(fs.size(), 1);
        auto f = fs[0];
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
        QVERIFY(af.stream().codec()->codec() != nullptr);

        auto format = af.format();
        QCOMPARE(format.sampleFormat(), QAVAudioFormat::Int32);
        auto data = af.data();
        QVERIFY(!data.isEmpty());

        QCOMPARE(d.eof(), false);
    }

    QVERIFY(p2);
    QVERIFY(p2.packet());
    QVERIFY(p2.duration() > 0);
    QVERIFY(p2.packet()->size > 0);
    QCOMPARE(p2.packet()->stream_index, d.currentAudioStreams().first().index());

    QVERIFY(f2);
    QVERIFY(f2.frame());
    QVERIFY(f2.pts() > 0);

    QCOMPARE(d.eof(), true);
    QVERIFY(d.seek(0) >= 0);
}

void tst_QAVDemuxer::supportedFormats()
{
    QAVDemuxer d;
    auto fmts = QAVDemuxer::supportedFormats();
    QVERIFY(!fmts.isEmpty());
    QVERIFY(!QAVDemuxer::supportedProtocols().isEmpty());
    if (fmts.contains(QLatin1String("v4l2"))) {
        QFileInfo file(QLatin1String("/dev/video0"));
        if (file.exists()) {
            d.setInputFormat(QLatin1String("v4l2"));
            QVERIFY(d.load(QLatin1String("/dev/video0")) >= 0);
            d.unload();
        }
    }

    d.setInputFormat(QLatin1String("v4l2"));
    QVERIFY(d.load(QLatin1String("/dev/dummy")) < 0);
    d.unload();

    d.setInputFormat({});
    QFileInfo file(testData("colors.mp4"));
    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    d.unload();
    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
}

void tst_QAVDemuxer::metadata()
{
    QAVDemuxer d;

    QFileInfo file(testData("colors.mp4"));

    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    QVERIFY(!d.metadata().isEmpty());
    QVERIFY(!d.currentVideoStreams().first().metadata().isEmpty());
    QVERIFY(!d.currentAudioStreams().first().metadata().isEmpty());
    for (auto &stream : d.availableAudioStreams())
        QVERIFY(!stream.metadata().isEmpty());
    for (auto &stream : d.availableVideoStreams())
        QVERIFY(!stream.metadata().isEmpty());
}

void tst_QAVDemuxer::videoCodecs()
{
    QAVDemuxer d;
    auto codecs = QAVDemuxer::supportedVideoCodecs();
    QVERIFY(!codecs.isEmpty());
    QFileInfo file(testData("colors.mp4"));
    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    d.unload();
    d.setInputVideoCodec("h264");
    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    d.unload();
    d.setInputVideoCodec("unknown");
    QVERIFY(d.load(file.absoluteFilePath()) < 0);
}

void tst_QAVDemuxer::inputOptions()
{
    QAVDemuxer d;
    QFileInfo file(testData("colors.mp4"));
    d.setInputOptions({{"user_agent", "QAVPlayer"}});
    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
}

QTEST_MAIN(tst_QAVDemuxer)
#include "tst_qavdemuxer.moc"
