/***************************************************************
 * Copyright (C) 2020, 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavdemuxer_p.h"
#include "qavmuxer.h"
#include "qavaudioframe.h"
#include "qavvideoframe.h"
#include "qaviodevice.h"
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
    void muxerWritePackets();
    void muxerWriteFrames();
    void muxerWriteSubtitles();
    void muxerEnqueue();
    void muxerEnqueueFramesFromMultiSources();
    void muxerEnqueueFramesFromDev();
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
    QAVPacket p;
    QVERIFY(!p);
    QVERIFY(d.read(p) < 0);
    QVERIFY(d.seek(0) < 0);

    QVERIFY(!p);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.packet()->size, 0);
    QVERIFY(p.packet()->stream_index < 0);
    QList<QAVFrame> fs;
    QAVDemuxer::decode(p, fs);
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
    QAVPacket p;
    QVERIFY(d.read(p) < 0);
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
    while (d.read(p) >= 0) {
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
        QAVDemuxer::decode(p, fs);
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

    QAVPacket p;
    QVERIFY(d.read(p) >= 0);
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.packet()->size > 0);
    QVERIFY(p.packet()->stream_index >= 0);

    QList<QAVFrame> fs;
    QAVDemuxer::decode(p, fs);
    QCOMPARE(fs.size(), 1);
    auto f = fs[0];
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d.eof(), false);

    QVERIFY(d.seek(0) >= 0);
    while (d.read(p) >= 0) {
        QCOMPARE(d.eof(), false);
        if (p.packet()->stream_index == d.currentVideoStreams().first().index()) {
            QList<QAVFrame> fs1;
            QAVDemuxer::decode(p, fs1);
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

    QSharedPointer<QIODevice> file(new QFile(testData("colors.mp4")));
    if (!file->open(QIODevice::ReadOnly)) {
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

    QAVPacket p;
    QVERIFY(d.read(p) >= 0);
    QVERIFY(p);
    QVERIFY(p.packet());
    QVERIFY(p.duration() > 0);
    QVERIFY(p.packet()->size > 0);
    QVERIFY(p.packet()->stream_index >= 0);

    QList<QAVFrame> fs;
    QAVDemuxer::decode(p, fs);
    QCOMPARE(fs.size(), 1);
    auto f = fs[0];
    QVERIFY(f);
    QVERIFY(f.frame());
    QVERIFY(f.pts() >= 0);
    QCOMPARE(d.eof(), false);

    QVERIFY(d.seek(0) >= 0);
    while (d.read(p) >= 0) {
        QCOMPARE(d.eof(), false);
        if (p.packet()->stream_index == d.currentVideoStreams().first().index()) {
            QList<QAVFrame> fs1;
            QAVDemuxer::decode(p, fs1);
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

    QSharedPointer<QIODevice> file(new QFile(":/test.wav"));
    if (!file->open(QIODevice::ReadOnly)) {
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
    while (d.read(p) >= 0) {
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
        QAVDemuxer::decode(p, fs);
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

void tst_QAVDemuxer::muxerWritePackets()
{
    QFileInfo file(testData("colors.mp4"));
    QAVDemuxer d;
    QAVMuxerPackets m;

    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    QVERIFY(m.load(d.availableStreams(), "colors.mkv") >= 0);

    QAVPacket p;
    while (d.read(p) >= 0) {
        QVERIFY(m.write(p) >= 0);
    }
    QVERIFY(m.flush() >= 0);
    m.unload();
    d.unload();
    QVERIFY(d.load("colors.mkv") >= 0);
}

void tst_QAVDemuxer::muxerWriteFrames()
{
    QFileInfo file(testData("colors.mp4"));
    QAVDemuxer d;
    QAVMuxerFrames m;

    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    QVERIFY(m.load(d.availableStreams(), "colors.mkv") >= 0);

    QAVPacket p;
    while (d.read(p) >= 0) {
        QList<QAVFrame> fs;
        QAVDemuxer::decode(p, fs);
        if (fs.size()) {
            QVERIFY(fs.size() == 1);
            auto &f = fs[0];
            QVERIFY(m.write(f) >= 0);
        }
    }
    QVERIFY(m.flush() >= 0);
    m.unload();
    d.unload();
    QVERIFY(d.load("colors.mkv") >= 0);
}

void tst_QAVDemuxer::muxerWriteSubtitles()
{
    QFileInfo file(testData("colors_subtitles.mkv"));
    QAVDemuxer d;
    QAVMuxerFrames m;

    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    QVERIFY(m.load(d.availableStreams(), "colors.mkv") >= 0);

    QAVPacket p;
    while (d.read(p) >= 0) {
        switch (p.stream().codec()->avctx()->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_AUDIO: {
            QList<QAVFrame> fs;
            QAVDemuxer::decode(p, fs);
            if (fs.size()) {
                QVERIFY(fs.size() == 1);
                auto &f = fs[0];
                QVERIFY(m.write(f) >= 0);
            }
            break;
        }
        case AVMEDIA_TYPE_SUBTITLE: {
            QList<QAVSubtitleFrame> fs;
            QAVDemuxer::decode(p, fs);
            if (fs.size()) {
                QVERIFY(fs.size() == 1);
                auto &f = fs[0];
                QVERIFY(m.write(f) >= 0);
            }
            break;
        }
        default:
            break;
        }
    }
    QVERIFY(m.flush() >= 0);
    m.unload();
    d.unload();
    QVERIFY(d.load("colors.mkv") >= 0);
    QVERIFY(!d.availableSubtitleStreams().isEmpty());
}

void tst_QAVDemuxer::muxerEnqueue()
{
    QFileInfo file(testData("colors.mp4"));
    QAVDemuxer d;
    QAVMuxerFrames m;

    QVERIFY(d.load(file.absoluteFilePath()) >= 0);
    QVERIFY(m.load(d.availableStreams(), "colors.mkv") >= 0);

    QAVPacket p;
    while (d.read(p) >= 0) {
        QList<QAVFrame> fs;
        QAVDemuxer::decode(p, fs);
        if (fs.size()) {
            QVERIFY(fs.size() == 1);
            auto &f = fs[0];
            m.enqueue(f);
        }
    }
    QTRY_VERIFY(m.size() == 0);
    QVERIFY(m.flush() >= 0);
    m.unload();
    d.unload();
    QVERIFY(d.load("colors.mkv") >= 0);
}

void tst_QAVDemuxer::muxerEnqueueFramesFromMultiSources()
{
    QFileInfo file1(testData("colors.mp4"));
    QFileInfo file2(testData("small.mp4"));
    QAVDemuxer d1;
    QAVDemuxer d2;
    QAVMuxerFrames m;

    QVERIFY(d1.load(file1.absoluteFilePath()) >= 0);
    QVERIFY(d2.load(file2.absoluteFilePath()) >= 0);
    auto streams = d1.availableStreams() + d2.availableStreams();
    QVERIFY(m.load(streams, "colors.mkv") >= 0);

    auto decode = [&](auto &p) {
        if (!p.stream())
            return;
        QList<QAVFrame> fs;
        QAVDemuxer::decode(p, fs);
        if (fs.size()) {
            QVERIFY(fs.size() == 1);
            auto &f = fs[0];
            m.enqueue(f);
        }
    };

    QAVPacket p1;
    QAVPacket p2;
    while (d1.read(p1) >= 0 || d2.read(p2) >= 0) {
        decode(p1);
        decode(p2);
    }
    QVERIFY(m.flush() >= 0);
    m.unload();
    d1.unload();
    d2.unload();
    QVERIFY(d1.load("colors.mkv") >= 0);
    QVERIFY(d1.availableStreams().size() == 4);
    QVERIFY(d1.availableVideoStreams().size() == 2);
    QVERIFY(d1.availableAudioStreams().size() == 2);
}

void tst_QAVDemuxer::muxerEnqueueFramesFromDev()
{
    QAVDemuxer d1;
    QAVDemuxer d2;
    QAVMuxerFrames m;
    auto fmts = QAVDemuxer::supportedFormats();
    QVERIFY(!fmts.isEmpty());
    QVERIFY(!QAVDemuxer::supportedProtocols().isEmpty());
    if (!fmts.contains(QLatin1String("v4l2")))
        return;
    QFileInfo file1(QLatin1String("/dev/video0"));
    if (!file1.exists())
        return;

    d1.setInputFormat(QLatin1String("v4l2"));
    QVERIFY(d1.load(QLatin1String("/dev/video0")) >= 0);
    QFileInfo file2(testData("small.mp4"));
    QVERIFY(d2.load(file2.absoluteFilePath()) >= 0);
    auto streams = d1.availableStreams() + d2.availableStreams();
    QVERIFY(m.load(streams, "output.mkv") >= 0);

    auto decode = [&](auto &p) {
        if (!p.stream())
            return;
        QList<QAVFrame> fs;
        QAVDemuxer::decode(p, fs);
        if (fs.size()) {
            QVERIFY(fs.size() == 1);
            auto &f = fs[0];
            m.enqueue(f);
        }
    };

    QAVPacket p1;
    QAVPacket p2;
    while (d1.read(p1) >= 0 && d2.read(p2) >= 0) {
        decode(p1);
        decode(p2);
    }

    QVERIFY(m.flush() >= 0);
    m.unload();
    p1 = {};
    p2 = {};
    d1.unload();
    d2.unload();
    QAVDemuxer d;
    QVERIFY(d.load("output.mkv") >= 0);
    QVERIFY(d.availableStreams().size() == 3);
    QVERIFY(d.availableVideoStreams().size() == 2);
    QVERIFY(d.availableAudioStreams().size() == 1);
}

QTEST_MAIN(tst_QAVDemuxer)
#include "tst_qavdemuxer.moc"
