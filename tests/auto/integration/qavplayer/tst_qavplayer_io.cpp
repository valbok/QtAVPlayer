/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"
#include "qaviodevice.h"

#include <QtTest/QtTest>

class Buffer: public QIODevice
{
public:
    qint64 readData(char *data, qint64 maxSize) override
    {
        if (!maxSize)
            return 0;

        QByteArray ba = m_buffer.mid(m_pos, maxSize);
        memcpy(data, ba.data(), ba.size());
        m_pos += ba.size();
        return ba.size();
    }

    qint64 writeData(const char *data, qint64 maxSize) override
    {
        QByteArray ba(data, maxSize);
        m_buffer.append(ba);
        emit readyRead();
        return ba.size();
    }

    bool atEnd() const override
    {
        return m_pos >= m_size;
    }

    qint64 pos() const override
    {
        return m_pos;
    }

    bool seek(qint64 pos) override
    {
        m_pos = pos;
        return true;
    }

    qint64 size() const override
    {
        return m_size;
    }

    qint64 m_size = 0;
    qint64 m_pos = 0;
    QByteArray m_buffer;
};

class BufferSequential : public Buffer
{
public:
    BufferSequential() = default;
    bool isSequential() const override
    {
        return true;
    }
};

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

void tst_QAVPlayer::filesIO_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("small") << testData("small.mp4");
    QTest::newRow("colors") << testData("colors.mp4");
    QTest::newRow("3_2ch_32k_bars_sine") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv");
}

void tst_QAVPlayer::filesIO()
{
    QFETCH(QString, path);

    QFileInfo fileInfo(path);
    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    QSharedPointer<Buffer> buffer(new Buffer);
    buffer->m_size = file.size();
    buffer->open(QIODevice::ReadWrite);

    QAVPlayer p;
    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QSharedPointer<QAVIODevice> dev(new QAVIODevice(buffer));
    p.setSource(fileInfo.fileName(), dev);
    p.play();

    while(!file.atEnd()) {
        auto bytes = file.read(64 * 1024);
        buffer->write(bytes);
        QTest::qWait(50);
    }

    QTRY_VERIFY(frame);
    QTRY_VERIFY(framesCount > 10);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 20000);
}

void tst_QAVPlayer::filesIOSequential_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("colors") << testData("colors.mp4");
    QTest::newRow("3_2ch_32k_bars_sine") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv");
}

void tst_QAVPlayer::filesIOSequential()
{
    QFETCH(QString, path);

    QFileInfo fileInfo(path);
    QFile file(fileInfo.absoluteFilePath());
    QVERIFY(file.open(QFile::ReadOnly));

    QSharedPointer<BufferSequential> buffer(new BufferSequential);
    buffer->m_size = file.size();
    buffer->open(QIODevice::ReadWrite);

    QAVPlayer p;
    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QSharedPointer<QAVIODevice> dev(new QAVIODevice(buffer));
    p.setSource(fileInfo.fileName(), dev);
    p.play();

    while(!file.atEnd()) {
        auto bytes = file.read(64 * 1024);
        buffer->write(bytes);
        QTest::qWait(50);
    }

    QTRY_VERIFY(frame);
    QTRY_VERIFY(framesCount > 10);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 20000);
}

void tst_QAVPlayer::subfile()
{
    QAVPlayer p;

    QFileInfo fileInfo(testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv"));
    QString src = QLatin1String("subfile,,start,0,end,0,,:") + fileInfo.absoluteFilePath();
    p.setSource(src);

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.play();
    QTRY_VERIFY(frame);
    QTRY_VERIFY(framesCount > 40);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
}

void tst_QAVPlayer::subfileTar()
{
    QAVPlayer p;

    QFileInfo fileInfo(testData("dv.tar"));
    QString src = QLatin1String("subfile,,start,1000,end,0,,:") + fileInfo.absoluteFilePath();
    p.setSource(src);

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.play();
    QTRY_VERIFY(frame);
    QTRY_VERIFY(framesCount > 5);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
}
