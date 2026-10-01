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

class BufferSequential : public Buffer
{
public:
    BufferSequential() = default;
    bool isSequential() const override
    {
        return true;
    }
};

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
