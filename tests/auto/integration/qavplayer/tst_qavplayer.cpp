/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"
#include "qavaudiooutput.h"
#include "qaviodevice_p.h"

#include <QDebug>
#include <QtTest/QtTest>

extern "C" {
#include <libavcodec/avcodec.h>
}

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "../testdata"
#endif

QT_USE_NAMESPACE

class tst_QAVPlayer : public QObject
{
    Q_OBJECT
public slots:
    void initTestCase();
    QString testData(const QString &fn) { return QLatin1String(TEST_DATA_DIR) + "/" + fn; }
private slots:
    void construction();
    void sourceChanged();
    void speedChanged();
    void quitAudio();
    void playIncorrectSource();
    void playAudio();
    void playAudioOutput();
    void pauseAudio();
    void stopAudio();
    void seekAudio();
    void speedAudio();
    void playVideo();
    void pauseVideo();
    void seekVideo();
    void seekVideoNegative();
    void speedVideo();
    void videoFrame();
    void pauseSeekVideo();
    void files_data();
    void files();
    void files_io_data();
    void files_io();
    void convert_data();
    void convert();
    void map_data();
    void map();
#ifdef QT_AVPLAYER_MULTIMEDIA
    void cast2QVideoFrame_data();
    void cast2QVideoFrame();
#endif
    void stepForward();
    void stepBackward();
    void availableAudioStreams();
#ifdef QT_AVPLAYER_MULTIMEDIA
    void audioOutput();
#endif
    void setEmptySource();
    void accurateSeek_data();
    void accurateSeek();
    void lastFrame();
    void configureFilter();
    void changeSourceFilter();
    void filter_data();
    void filter();
    void filesIO_data();
    void filesIO();
    void filesIOSequential_data();
    void filesIOSequential();
    void subfile();
    void subfileTar();
    void subtitles();
    void synced();
    void bsf();
    void bsfInvalid();
    void convertDirectConnection();
    void mapTwice();
    void changeFormat();
    void filterName();
    void filterNameStep();
    void audioVideoFilter();
    void audioFilterVideoFrames();
    void multipleFilters();
    void multipleAudioVideoFilters();
    void inputFormat();
    void inputVideoCodec();
    void flushFilters();
    void multipleAudioStreams();
    void multipleVideoStreams_data();
    void multipleVideoStreams();
    void emptyStreams();
    void flushCodecs();
    void multiFilterInputs_data();
    void multiFilterInputs();
};

void tst_QAVPlayer::initTestCase()
{
    QThreadPool::globalInstance()->setMaxThreadCount(20);
}

void tst_QAVPlayer::construction()
{
    QAVPlayer p;
    QVERIFY(p.source().isEmpty());
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(p.availableSubtitleStreams().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.speed(), 1.0);
    QVERIFY(!p.isSeekable());
}

void tst_QAVPlayer::sourceChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::sourceChanged);
    p.setSource(QLatin1String("unknown.mp4"));
    QCOMPARE(spy.count(), 1);
    p.setSource(QLatin1String("unknown.mp4"));
    QCOMPARE(spy.count(), 1);
}

void tst_QAVPlayer::speedChanged()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::speedChanged);
    QCOMPARE(p.speed(), 1.0);
    p.setSpeed(0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.speed(), 0.0);
    p.setSpeed(2);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(p.speed(), 2.0);
}

void tst_QAVPlayer::quitAudio()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
}

void tst_QAVPlayer::playIncorrectSource()
{
    QAVPlayer p;
    QSignalSpy spyStateChanged(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QCOMPARE(spyStateChanged.count(), 0);

    p.setSource("unknown");
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spyErrorOccurred.clear();

    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QCOMPARE(spyStateChanged.count(), 0);
    QCOMPARE(spyErrorOccurred.count(), 0);

    spyStateChanged.clear();

    p.setSource("unknown");
    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);
    QCOMPARE(spyStateChanged.count(), 0);
    QCOMPARE(spyErrorOccurred.count(), 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(spyStateChanged.count(), 1);
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
}

void tst_QAVPlayer::playAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(p.duration(), 999);
    QCOMPARE(p.position(), 0);
    QVERIFY(!p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(p.isSeekable());

    spyState.clear();
    spyMediaStatus.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QTRY_VERIFY(p.position() != 0);
    QCOMPARE(spyMediaStatus.count(), 0);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();

    p.play();

    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> Loaded
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(p.duration(), 999);

    QTRY_VERIFY(p.position() != 0);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());

    p.setSource({});

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
}

void tst_QAVPlayer::playAudioOutput()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    QAVAudioFrame frame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frame = f; });
    p.play();

    QTRY_VERIFY(p.position() != 0);
    QTRY_VERIFY(frame);
    QCOMPARE(frame.format().sampleFormat(), QAVAudioFormat::Int32);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
}

void tst_QAVPlayer::pauseAudio()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);

    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);

    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(spyState.count(), 2); // Stopped -> Playing -> Paused
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QTRY_COMPARE(spyPaused.count(), 1);

    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 1);
}

void tst_QAVPlayer::stopAudio()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());
    p.play();

    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(p.position() != 0);

    p.stop();

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() != 0);
    QTRY_VERIFY(p.duration() != 0);

    p.play();

    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::seekAudio()
{
    QAVPlayer p;
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    p.seek(500);
    QTRY_COMPARE(p.position(), 500);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(p.position() >= 500);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(100);
    QCOMPARE(p.position(), 100);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.position(), p.duration());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);

    p.seek(100000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.position(), p.duration());

    p.seek(200);
    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() > 200);

    p.seek(100);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(p.position() < 200);

    p.seek(p.duration());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);
}

void tst_QAVPlayer::speedAudio()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    p.setSpeed(0.5);
    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.speed(), 0.5);
    QTest::qWait(50);
    p.setSpeed(2.0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::playVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);
    QSignalSpy spyVideoFrameRateChanged(&p, &QAVPlayer::videoFrameRateChanged);

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QVERIFY(p.availableAudioStreams().isEmpty());
    QVERIFY(p.availableVideoStreams().isEmpty());
    QVERIFY(!p.isSeekable());
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 0);
    QCOMPARE(spyDuration.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 0);
    QCOMPARE(p.videoFrameRate(), 0.0);

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // NoMedia -> Loaded
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 1);
    QCOMPARE(p.videoFrameRate(), 0.04);
    QVERIFY(qAbs(p.duration() - 15019) < 2);
    QVERIFY(!p.availableAudioStreams().isEmpty());
    QVERIFY(!p.availableVideoStreams().isEmpty());
    QCOMPARE(p.currentVideoStreams().first().framesCount(), 375);
    QCOMPARE(p.currentVideoStreams().first().frameRate(), 0.04);
    QCOMPARE(p.currentAudioStreams().first().framesCount(), 704);
    QVERIFY(p.isSeekable());
    QCOMPARE(p.position(), 0);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyVideoFrameRateChanged.count(), 1);

    QTRY_VERIFY(p.position() != 0);
}

void tst_QAVPlayer::pauseVideo()
{
    QAVPlayer p;

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
}

void tst_QAVPlayer::seekVideo()
{
    QAVPlayer p;
    QSignalSpy spyState(&p, &QAVPlayer::stateChanged);
    QSignalSpy spyMediaStatus(&p, &QAVPlayer::mediaStatusChanged);
    QSignalSpy spyDuration(&p, &QAVPlayer::durationChanged);
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);
    QSignalSpy spyPaused(&p, &QAVPlayer::paused);
    QSignalSpy spyStopped(&p, &QAVPlayer::stopped);

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame, &framesCount](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    p.seek(14500);
    p.play();
    QTRY_COMPARE(spyStopped.count(), 1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
    QVERIFY(!p.availableVideoStreams().isEmpty());
    QVERIFY(!p.availableAudioStreams().isEmpty());
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(spyPaused.count(), 0);
    QVERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spySeeked.clear();
    spyStopped.clear();

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(frame == true);
    QVERIFY(framesCount > 0);
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QCOMPARE(spyMediaStatus.count(), 1); // EndOfMedia -> Loaded
    QCOMPARE(spyDuration.count(), 0);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 80); // Playing from beginning
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QVERIFY(qAbs(p.duration() - 15019) < 2);

    QTRY_VERIFY(p.position() < 5000 && p.position() > 1000);

    spyMediaStatus.clear();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.position() != p.duration());
    QCOMPARE(spyPaused.count(), 0);
    QTRY_COMPARE(spyStopped.count(), 1);
    QCOMPARE(spyMediaStatus.count(), 0);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spySeeked.clear();
    framesCount = 0;

    p.setSource({});
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::NoMedia);
    QCOMPARE(p.duration(), 0);
    QCOMPARE(p.position(), 0);
    QCOMPARE(spyState.count(), 0);
    QCOMPARE(spyMediaStatus.count(), 1); // Loaded -> NoMedia
    QCOMPARE(spyDuration.count(), 1);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    spyState.clear();
    spyMediaStatus.clear();
    spyDuration.clear();
    spyState.clear();
    spyStopped.clear();

    p.setSource(file.absoluteFilePath());
    p.play();
    QTRY_COMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_COMPARE(spyMediaStatus.count(), 1); // NoMeida -> Loaded
    QCOMPARE(spyState.count(), 1); // Stopped -> Playing
    QTRY_COMPARE(spyDuration.count(), 1);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(spyStopped.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == true);
    QTRY_VERIFY(framesCount > 0);

    spyStopped.clear();

    p.seek(14500);
    QTRY_COMPARE(spyStopped.count(), 1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(p.position() != p.duration());
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(frame == true);

    framesCount = 0;
    QTest::qWait(10);
    spyStopped.clear();

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QTRY_COMPARE(spyStopped.count(), 1);
    QTRY_COMPARE(seekPosition, 0);

    spySeeked.clear();
    spyStopped.clear();
    seekPosition = -1;

    p.seek(0);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(seekPosition < 10);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    const int pos = p.position();
    QTest::qWait(50);
    QCOMPARE(p.position(), pos);
    p.seek(0);
    QCOMPARE(p.position(), pos);
    QTest::qWait(50);
    QCOMPARE(p.position(), pos);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(spySeeked.count(), 2);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);

    spySeeked.clear();

    p.seek(5000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    p.play();

    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 5000, 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 0);
    QTRY_VERIFY(qAbs(seekPosition - 5000) < 500);
    seekPosition = -1;

    p.seek(8000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY_WITH_TIMEOUT(p.position() > 8000, 10000);
    QTRY_COMPARE(spySeeked.count(), 2);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 8000) < 500);
    seekPosition = -1;

    p.seek(2000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QCOMPARE(p.position(), 2000);
    QTRY_COMPARE(spySeeked.count(), 3);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 2000) < 500);
    QTRY_VERIFY(frame == true);
    seekPosition = -1;
    framesCount = 0;

    p.stop();
    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);
    QTRY_COMPARE(spyStopped.count(), 1);
    QCOMPARE(pausePosition, -1);

    spyPaused.clear();

    QTest::qWait(50);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_COMPARE(spyPaused.count(), 1);
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    spyPaused.clear();
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    p.pause();
    QTest::qWait(50);
    QCOMPARE(spyPaused.count(), 0);
    QCOMPARE(pausePosition, -1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);

    p.seek(14500);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(qAbs(seekPosition - 14500) < 500);
    seekPosition = -1;
    QCOMPARE(spyPaused.count(), 0);

    spyStopped.clear();

    p.play();
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(spyStopped.count(), 1, 10000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spySeeked.count(), 4);
    QCOMPARE(seekPosition, -1);
    QCOMPARE(spyPaused.count(), 0);
    p.play();
}

void tst_QAVPlayer::seekVideoNegative()
{
    QAVPlayer p;
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame, &framesCount](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    p.seek(-1000);
    QCOMPARE(p.position(), -1000);

    p.play();
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(qAbs(seekPosition - 14400) < 500);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-50000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(p.duration() - 5000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition > 0);
    qint64 sp = seekPosition;

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-5000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(qAbs(seekPosition - sp) < 1000);

    spySeeked.clear();
    seekPosition = -1;

    p.pause();
    p.seek(0);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-1000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= 13500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-2000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 2500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-20000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.stop();
    p.seek(-3000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 3500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-14000);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 14500);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(-500);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition >= p.duration() - 1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    spySeeked.clear();
    seekPosition = -1;

    p.play();
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(seekPosition, -1);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(0);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_VERIFY(seekPosition < 500);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    p.stop();

    spySeeked.clear();
    seekPosition = -1;

    p.seek(p.duration());
    p.play();
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_VERIFY(qAbs(seekPosition - p.duration()) < 500);

    spySeeked.clear();
    seekPosition = -1;

    p.seek(p.duration() + 1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spySeeked.count(), 0);
    QCOMPARE(seekPosition, -1);
}

void tst_QAVPlayer::speedVideo()
{
    QAVPlayer p;

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.setSpeed(5);
    p.play();

    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);

    p.setSpeed(0.5);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.speed(), 0.5);

    p.play();

    QTest::qWait(100);
    QTRY_VERIFY(frame);
    p.setSpeed(5);
    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(frame);

    p.play();
}

void tst_QAVPlayer::videoFrame()
{
    QAVPlayer p;

    QFileInfo file(testData("colors.mp4"));
    p.setSource(file.absoluteFilePath());

    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.play();
    QTRY_VERIFY(!frame.size().isEmpty());
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);

    p.stop();
}

void tst_QAVPlayer::pauseSeekVideo()
{
    QAVPlayer p;

    QFileInfo file(testData("colors.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; if (f) ++framesCount; });
    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });
    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });

    p.setSource(file.absoluteFilePath());
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(frame.size().isEmpty());
    QCOMPARE(pausePosition, -1);

    p.pause();
    QCOMPARE(framesCount, 0);
    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;

    QCOMPARE(p.state(), QAVPlayer::PausedState);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QTest::qWait(100);
    QVERIFY(framesCount < 2);
    QCOMPARE(pausePosition, -1);

    p.pause();

    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    int count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(pausePosition, -1);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 5);
    QTRY_VERIFY(frame);
    QTRY_VERIFY(seekPosition >= 0);
    QVERIFY(seekPosition < 220);
    seekPosition = -1;

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTest::qWait(100);
    QVERIFY(framesCount < 2);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(pausePosition, -1);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 5);
    QVERIFY(frame);
    QTRY_VERIFY(seekPosition >= 0);
    QTRY_VERIFY(qAbs(seekPosition - 1) < 200);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 5);
    QVERIFY(frame);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QTRY_VERIFY(count != framesCount);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QTRY_VERIFY(count != framesCount);
    QTRY_VERIFY(seekPosition > 0);
    QTRY_VERIFY(seekPosition < 1500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.stop();
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(framesCount, 0);
    QTest::qWait(100);
    QVERIFY(framesCount >= 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 5);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_VERIFY(!frame.size().isEmpty());
    QVERIFY(framesCount > 0);
    QTest::qWait(100);
    QTRY_VERIFY(frame);
    QTRY_VERIFY(seekPosition > 0);
    QTRY_VERIFY(seekPosition < 1500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QVERIFY(frame.size().isEmpty());
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTRY_COMPARE(pausePosition, p.position());
    pausePosition = -1;
    QTest::qWait(100);
    QTRY_VERIFY(framesCount - count < 2);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QCOMPARE(pausePosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QCOMPARE(pausePosition, -1);
    QTRY_VERIFY(!frame.size().isEmpty());
    QCOMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(framesCount > 0);
    count = framesCount;
    QTest::qWait(100);
    QVERIFY(framesCount - count < 2);
    QTRY_VERIFY(seekPosition > 0);
    QTRY_VERIFY(seekPosition < 1500);
    seekPosition = -1;
    QCOMPARE(pausePosition, -1);
}

void tst_QAVPlayer::files_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<int>("duration");
    QTest::addColumn<bool>("hasVideo");
    QTest::addColumn<bool>("hasAudio");

    QTest::newRow("test.wav") << testData("test.wav") << 999 << false << true;
    QTest::newRow("colors.mp4") << testData("colors.mp4") << 15019 << true << true;
    QTest::newRow("shots0000.dv") << testData("shots0000.dv") << 40 << true << false;
    QTest::newRow("dv_dsf_1_stype_1.dv") << testData("dv_dsf_1_stype_1.dv") << 600 << true << true;
    QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << 2000 << true << true;
    QTest::newRow("small.mp4") << testData("small.mp4") << 5568 << true << true;
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov") << 6840 << true << false;
    QTest::newRow("star_trails.mpeg") << testData("star_trails.mpeg") << 1050 << true << true;
}

void tst_QAVPlayer::files()
{
    QFETCH(QString, path);
    QFETCH(int, duration);
    QFETCH(bool, hasVideo);
    QFETCH(bool, hasAudio);

    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(file.absoluteFilePath());

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
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 18000);
    for (const auto &s : p.availableVideoStreams()) {
        auto progress = p.progress(s);
        QVERIFY(progress.pts() >= 0.0);
        QVERIFY(progress.framesCount() > 0);
        QVERIFY(progress.frameRate() > 0.0);
        QVERIFY(progress.fps() > 0);
    }
}

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
    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        QFAIL("Could not open");
        return;
    }

    p.setSource(path, &file);

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
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 18000);
}

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
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&frame](const QAVVideoFrame &f) { frame = f; });

    p.play();
    QTRY_VERIFY(frame);

    auto mapData = frame.map();
    QVERIFY(mapData.size > 0);
    QVERIFY(mapData.bytesPerLine[0] > 0);
    QVERIFY(mapData.bytesPerLine[1] > 0);
    QVERIFY(mapData.data[0] != nullptr);
    QVERIFY(mapData.data[1] != nullptr);
}

#ifdef QT_AVPLAYER_MULTIMEDIA
void tst_QAVPlayer::cast2QVideoFrame_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QSize>("size");

    QTest::newRow("colors.mp4") << testData("colors.mp4") << QSize(160, 120);
    QTest::newRow("dv_dsf_1_stype_1.dv") << testData("dv_dsf_1_stype_1.dv") << QSize(720, 576);
    QTest::newRow("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv") << QSize(720, 576);
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
}
#endif // #ifdef QT_AVPLAYER_MULTIMEDIA

void tst_QAVPlayer::stepForward()
{
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });
    qint64 pausePosition = -1;
    QObject::connect(&p, &QAVPlayer::paused, &p, [&](qint64 pos) { pausePosition = pos; });
    qint64 stepPosition = -1;
    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64 pos) { stepPosition = pos; });

    p.setSource(file.absoluteFilePath());
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(!frame);
    QCOMPARE(pausePosition, -1);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(!frame);
    QCOMPARE(framesCount, 0);
    QTest::qWait(50);

    p.pause();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QVERIFY(frame == false);
    QCOMPARE(framesCount, 0);
    QCOMPARE(stepPosition, -1);

    qint64 prev = -1;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    p.play();
    QCOMPARE(p.state(), QAVPlayer::PlayingState);
    QTest::qWait(50);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTest::qWait(50);
    QTRY_VERIFY(frame);
    QTRY_VERIFY(stepPosition > prev);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    framesCount = 0;
    stepPosition = -1;

    p.stop();
    QVERIFY(framesCount < 3);
    QCOMPARE(stepPosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;
    stepPosition = -1;

    QTest::qWait(50);
    p.stop();
    QCOMPARE(stepPosition, -1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;

    p.stepForward();
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QTRY_VERIFY(stepPosition > prev);

    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64) { p.stepForward(); });
    bool eof = false;
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, &p, [&](QAVPlayer::MediaStatus s) { if (!eof) eof = s == QAVPlayer::EndOfMedia; });
    p.stepForward();
    QTRY_VERIFY_WITH_TIMEOUT(eof, 15000);
}

void tst_QAVPlayer::stepBackward()
{
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });
    qint64 stepPosition = -1;
    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64 pos) { stepPosition = pos; });
    QSignalSpy spySeeked(&p, &QAVPlayer::seeked);

    p.setSource(file.absoluteFilePath());
    QTest::qWait(200);
    QCOMPARE(framesCount, 0);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QVERIFY(!frame);

    qint64 prev = 2500;
    p.seek(prev);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;

    qint64 duration = p.videoFrameRate() * 1000;
    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2466);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QCOMPARE(p.state(), QAVPlayer::PausedState);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2433);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2400);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2366);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 2333);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    spySeeked.clear();
    framesCount = 0;
    prev = 100;

    p.seek(prev);
    QTRY_COMPARE(spySeeked.count(), 1);
    QTRY_COMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 66);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 33);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 0);
    QTRY_VERIFY(stepPosition - (prev - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5500);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5466);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5433);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    p.stepBackward();
    QTRY_COMPARE(stepPosition, 5400);
    QTRY_VERIFY(stepPosition - (p.duration() - duration) < 0.1);
    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;

    spySeeked.clear();
    prev = 100;
    p.seek(prev);
    QTRY_COMPARE(spySeeked.count(), 1);

    frame = QAVVideoFrame();
    framesCount = 0;
    prev = stepPosition;
    stepPosition = -1;
    QObject::connect(&p, &QAVPlayer::stepped, &p, [&](qint64) { p.stepBackward(); });
    p.stepForward();
    QTRY_COMPARE(stepPosition, 0);
    QVERIFY(stepPosition < p.duration());
}

void tst_QAVPlayer::availableAudioStreams()
{
    QAVPlayer p;

    QFileInfo file(testData("guido.mp4"));

    QSignalSpy spy(&p, &QAVPlayer::audioStreamsChanged);

    int framesCount = 0;
    QAVAudioFrame frame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frame = f; ++framesCount; });

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
void tst_QAVPlayer::audioOutput()
{
    QFileInfo file1(testData("guido.mp4"));
    QFileInfo file2(testData("small.mp4"));

    QAVPlayer p;
    QAVAudioOutput out;
    QObject::connect(&p, &QAVPlayer::audioFrame, &out, [&out](const QAVAudioFrame &f) { out.play(f); });

    p.setSource(file1.absoluteFilePath());
    p.play();
    QTest::qWait(100);

    p.setSource(file2.absoluteFilePath());
    p.play();
    QTRY_VERIFY(p.position() > 500);
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

void tst_QAVPlayer::accurateSeek_data()
{
    QTest::addColumn<QString>("path");

    QTest::newRow("colors.mp4") << testData("colors.mp4");
    QTest::newRow("small.mp4") << testData("small.mp4");
    QTest::newRow("Earth_Zoom_In.mov") << testData("Earth_Zoom_In.mov");
    QTest::newRow("test6g100.mkv") << testData("test6g100.mkv");
}

void tst_QAVPlayer::accurateSeek()
{
    QFETCH(QString, path);
    QAVPlayer p;

    QFileInfo file(path);
    p.setSource(file.absoluteFilePath());

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    p.seek(5000);
    QTRY_COMPARE(seekPosition, 5000);
    QTRY_VERIFY(framesCount < 3);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(frame.pts(), 5.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(4000);
    QTRY_COMPARE(seekPosition, 4000);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 4.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(3000);
    QTRY_COMPARE(seekPosition, 3000);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 3.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(2000);
    QTRY_COMPARE(seekPosition, 2000);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 2.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(1000);
    QTRY_COMPARE(seekPosition, 1000);
    QTRY_VERIFY(framesCount < 3);
    QCOMPARE(frame.pts(), 1.0);

    frame = QAVVideoFrame();
    framesCount = 0;

    p.seek(0);
    QTRY_COMPARE(seekPosition, 0);
    QTRY_VERIFY(framesCount < 3);
    QTRY_COMPARE(frame.pts(), 0.0);

    seekPosition = -1;
    frame = QAVVideoFrame();

    p.play();
    p.seek(2000);
    QTRY_COMPARE(seekPosition, 2000);

    p.seek(1000);
    QTRY_COMPARE(seekPosition, 1000);

    p.seek(3000);
    QTRY_COMPARE(seekPosition, 3000);

    p.seek(5000);
    QTRY_COMPARE(seekPosition, 5000);

    p.seek(4000);
    QTRY_COMPARE(seekPosition, 4000);

    QTest::qWait(100);

    p.seek(0);
    QTRY_COMPARE(seekPosition, 0);

    QTest::qWait(100);

    p.seek(1500);
    QTRY_VERIFY(qAbs(seekPosition - 1500) < 30);

    QTest::qWait(100);

    p.seek(4500);
    QTRY_VERIFY(qAbs(seekPosition - 4500) < 30);

    QTest::qWait(100);

    p.seek(3500);
    QTRY_VERIFY(qAbs(seekPosition - 3500) < 30);

    p.seek(2500);
    QTRY_VERIFY(qAbs(seekPosition - 2500) < 30);

    p.pause();

    p.seek(1600);
    QTRY_VERIFY(qAbs(seekPosition - 1600) < 30);

    p.seek(2600);
    QTRY_VERIFY(qAbs(seekPosition - 2600) < 30);

    p.seek(4600);
    QTRY_VERIFY(qAbs(seekPosition - 4600) < 30);

    QTest::qWait(100);

    p.seek(500);
    QTRY_VERIFY(qAbs(seekPosition - 500) < 30);

    p.seek(0);
    QTRY_COMPARE(seekPosition, 0);

    seekPosition = INT_MIN;

    p.play();
    p.seek(1234);
    QTRY_VERIFY(qAbs(seekPosition - 1234) < 50);

    seekPosition = INT_MIN;

    p.seek(2345);
    QTRY_VERIFY(qAbs(seekPosition - 2345) < 50);

    seekPosition = INT_MIN;

    p.seek(5321);
    QTRY_VERIFY(qAbs(seekPosition - 5321) < 50);

    seekPosition = INT_MIN;

    p.seek(100);
    QTRY_VERIFY(qAbs(seekPosition - 100) < 50);

    seekPosition = INT_MIN;

    p.seek(200);
    QTRY_VERIFY(qAbs(seekPosition - 200) < 50);

    seekPosition = INT_MIN;

    p.seek(999);
    QTRY_VERIFY(qAbs(seekPosition - 999) < 50);

    seekPosition = INT_MIN;

    p.seek(123);
    QTRY_VERIFY(qAbs(seekPosition - 123) < 50);

    seekPosition = INT_MIN;

    p.seek(321);
    QTRY_VERIFY(qAbs(seekPosition - 321) < 50);

    seekPosition = INT_MIN;

    p.seek(666);
    QTRY_VERIFY(qAbs(seekPosition - 666) < 50);

    seekPosition = INT_MIN;

    p.seek(10);
    QTRY_VERIFY(qAbs(seekPosition - 10) < 50);

    seekPosition = INT_MIN;

    p.seek(1);
    QTRY_VERIFY(qAbs(seekPosition - 1) < 50);

    seekPosition = INT_MIN;

    p.seek(p.duration() - 1000);
    QTRY_VERIFY(qAbs(seekPosition - p.duration() + 1000) < 50);

    seekPosition = INT_MIN;

    p.seek(-1000);
    QTRY_VERIFY(qAbs(seekPosition - p.duration() + 1000) < 50);

    seekPosition = INT_MIN;

    p.seek(p.duration());
    QTRY_VERIFY(qAbs(seekPosition - p.duration()) < 100);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::lastFrame()
{
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));
    p.setSource(file.absoluteFilePath());

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    qint64 seekPosition = -1;
    QObject::connect(&p, &QAVPlayer::seeked, &p, [&](qint64 pos) { seekPosition = pos; });

    p.play();
    p.seek(100000);

    QTRY_VERIFY(frame);
    QCOMPARE(framesCount, 1);
    QCOMPARE(seekPosition, 5500);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);

    framesCount = 0;
    seekPosition = -1;

    p.seek(0);
    QTRY_COMPARE(framesCount, 1);

    p.play();
    framesCount = 0;
    seekPosition = -1;

    p.seek(p.duration());
    QTRY_COMPARE(framesCount, 1);
    QVERIFY(frame);
    QTRY_COMPARE(seekPosition, 5500);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::configureFilter()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;

    QFileInfo file(testData("small.mp4"));
    p.setSource(file.absoluteFilePath());
    QSignalSpy spy(&p, &QAVPlayer::filtersChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; });

    p.pause();
    QTRY_VERIFY(frame);
    QCOMPARE(frame.size(), QSize(560, 320));

    frame = QAVVideoFrame();

    QString desc = "scale=iw/2:-1";
    p.setFilter(desc);
    QCOMPARE(p.filters(), {desc});
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.play();
    QTRY_VERIFY(frame);
    QTRY_COMPARE(frame.size(), QSize(560 / 2, 320 / 2));

    spy.clear();
    spyErrorOccurred.clear();

    p.stop();
    p.setFilter(desc);
    QCOMPARE(p.filters(), {desc});
    QCOMPARE(spy.count(), 0);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.setFilter("");
    QCOMPARE(p.filters(), {});
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spyErrorOccurred.count(), 0);

    frame = QAVVideoFrame();

    p.pause();
    QTRY_VERIFY(frame);

    frame = QAVVideoFrame();

    p.play();
    QTRY_VERIFY(frame);
    QTRY_COMPARE(frame.size(), QSize(560, 320));

    p.stop();
    p.setFilter("wrong");
    QCOMPARE(p.filters(), {"wrong"});
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spyErrorOccurred.clear();

    p.pause();
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    frame = QAVVideoFrame();
    spyErrorOccurred.clear();

    p.pause();
    QCOMPARE(p.filters(), {"wrong"});
    QTRY_COMPARE(spyErrorOccurred.count(), 1);
    QVERIFY(!frame);

    spy.clear();
    spyErrorOccurred.clear();

    p.setFilter(desc);
    QCOMPARE(p.filters(), {desc});
    QTRY_COMPARE(spy.count(), 1);

    p.pause();
    QTRY_VERIFY(frame);
    QCOMPARE(spyErrorOccurred.count(), 0);

    spy.clear();
    spyErrorOccurred.clear();

    p.setFilter("wrong");
    QCOMPARE(p.filters(), {"wrong"});
    QTRY_COMPARE(spy.count(), 1);
    QTRY_COMPARE(spyErrorOccurred.count(), 1);
    QCOMPARE(p.state(), QAVPlayer::StoppedState);

    spyErrorOccurred.clear();

    p.pause();
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spyErrorOccurred.clear();

    p.stepForward();
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spy.clear();
    spyErrorOccurred.clear();

    p.setFilter("wrong2");
    QCOMPARE(p.filters(), {"wrong2"});
    QTRY_COMPARE(spy.count(), 1);
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spyErrorOccurred.clear();

    p.stop();
    QCOMPARE(spyErrorOccurred.count(), 1);

    spyErrorOccurred.clear();

    p.play();
    QTRY_COMPARE(spyErrorOccurred.count(), 1);

    spy.clear();
    spyErrorOccurred.clear();
    frame = QAVVideoFrame();

    p.setFilter("");
    QCOMPARE(p.filters(), {});
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.play();
    QTRY_VERIFY(frame);
    QTRY_COMPARE(frame.size(), QSize(560, 320));
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.setFilter(desc);
    QTRY_COMPARE(frame.size(), QSize(560 / 2, 320 / 2));

    p.seek(p.duration());
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
}

void tst_QAVPlayer::changeSourceFilter()
{
    QAVPlayer p;

    QFileInfo file1(testData("small.mp4"));
    p.setSource(file1.absoluteFilePath());
    QSignalSpy spy(&p, &QAVPlayer::filtersChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; });

    const QString desc = "scale=iw/2:-1";
    p.setFilter(desc);
    p.play();
    QTRY_VERIFY(frame);
    QCOMPARE(frame.size(), QSize(560 / 2, 320 / 2));

    QFileInfo file2(testData("colors.mp4"));
    p.setSource(file2.absoluteFilePath());

    QCOMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.filters(), {desc});

    frame = QAVVideoFrame();

    p.play();
    QTRY_COMPARE(frame.size(), QSize(160 / 2, 120 / 2));

    p.setSource(file1.absoluteFilePath());
    p.play();
    QTRY_COMPARE(frame.size(), QSize(560 / 2, 320 / 2));
}

void tst_QAVPlayer::filter_data()
{
    QTest::addColumn<QString>("filter");

    QTest::newRow("fps=fps=10") << QString("fps=fps=10");
    QTest::newRow("curves=vintage") << QString("curves=vintage");
    QTest::newRow("scale=1920:1080,setsar=1:1") << QString("scale=1920:1080,setsar=1:1");
    QTest::newRow("mirror") << QString("crop=iw/2:ih:0:0,split[left][tmp];[tmp]hflip[right];[left][right] hstack");
    QTest::newRow("split") << QString("split=4[a][b][c][d];[b]lutrgb=g=0:b=0[x];[c]lutrgb=r=0:b=0[y];[d]lutrgb=r=0:g=0[z];[a][x][y][z]hstack=4");
    QTest::newRow("yuv") << QString("split=4[a][b][c][d];[b]lutyuv=u=128:v=128[x];[c]lutyuv=y=0:v=128[y];[d]lutyuv=y=0:u=128[z];[a][x][y][z]hstack=4");
    QTest::newRow("histogram") << QString("format=gbrp,split=4[a][b][c][d],[d]histogram=display_mode=0:level_height=244[dd],[a]waveform=m=1:d=0:r=0:c=7[aa],[b]waveform=m=0:d=0:r=0:c=7[bb],[c][aa]vstack[V],[bb][dd]vstack[V2],[V][V2]hstack");
    QTest::newRow("vectorscope") << QString("format=yuv422p,split=4[a][b][c][d],[a]waveform[aa],[b][aa]vstack[V],[c]waveform=m=0[cc],[d]vectorscope=color4[dd],[cc][dd]vstack[V2],[V][V2]hstack");
    //QTest::newRow("waveform") << QString("split[a][b];[a]format=gray,waveform,split[c][d];[b]pad=iw:ih+256[padded];[c]geq=g=1:b=1[red];[d]geq=r=1:b=1,crop=in_w:220:0:16[mid];[red][mid]overlay=0:16[wave];[padded][wave]overlay=0:H-h");
    QTest::newRow("envelope") << QString("split[a][b];[a]waveform=e=3,split=3[c][d][e];[e]crop=in_w:20:0:235,lutyuv=v=180[low];[c]crop=in_w:16:0:0,lutyuv=y=val:v=180[high];[d]crop=in_w:220:0:16,lutyuv=v=110[mid] ; [b][high][mid][low]vstack=4");
    QTest::newRow("yuv420p10le") << QString("format=yuv420p10le|yuv422p10le|yuv444p10le|yuv440p10le,            lutyuv=                y=if(eq(1\\,-1)\\,512\\,if(eq(1\\,0)\\,val\\,bitand(val\\,pow(2\\,10-1))*pow(2\\,1))):                u=if(eq(-1\\,-1)\\,512\\,if(eq(-1\\,0)\\,val\\,bitand(val\\,pow(2\\,10--1))*pow(2\\,-1))):                v=if(eq(-1\\,-1)\\,512\\,if(eq(-1\\,0)\\,val\\,bitand(val\\,pow(2\\,10--1))*pow(2\\,-1))),format=yuv444p");
    QTest::newRow("bitplanenoise") << QString("bitplanenoise=bitplane=1:filter=1,extractplanes=y,format=yuv444p");
    QTest::newRow("lutyuv") << QString("lutyuv=y=if(gt(val\\,maxval)\\,val-maxval\\,0):u=(maxval+minval)/2:v=(maxval+minval)/2,histeq=strength=1");
    QTest::newRow("signalstats") << QString("signalstats=out=brng:c=0x40e0d0,format=yuv444p|rgb24");
    QTest::newRow("ciescope") << QString("ciescope=system=1:gamuts=pow(2\\,1):contrast=0.7:intensity=0.01");
    QTest::newRow("extractplanes") << QString("format=yuv444p,split[y][u];[y]extractplanes=y,pad=w=iw+256:h=ih:x=128,format=yuv444p[y1];[u]extractplanes=u,histeq,pad=w=iw+256:h=ih:x=0+128:y=0,format=yuv444p[u1];[y1][u1]vstack,il=l=i:c=i");
    QTest::newRow("crop") << QString("split[a][b];[a]crop=360:576:0:0[a1];[b]colormatrix=bt601:bt709[b1];[b1][a1]overlay");
    QTest::newRow("crop_histeq") << QString("split=4[a][b][c][d];[a]crop=w=24:h=24:x=0:y=0,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[a1];[b]crop=w=24:h=24:x=iw-24:y=0,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[b1];[c]crop=w=24:h=24:x=0:y=ih-24,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[c1];[d]crop=w=24:h=24:x=iw-24:y=ih-24,histeq=strength=0,scale=24*16:24*16:flags=neighbor,drawgrid=w=iw/24:h=ih/24:t=1:c=green@0.5[d1];[a1][b1]hstack[ab];[c1][d1]hstack[cd];[ab][cd]vstack,setsar=1/1,drawgrid=w=iw/2:h=ih/2:t=2:c=blue@0.5");
    QTest::newRow("datascope") << QString("datascope=x=0:y=0:mode=1:axis=1");
    QTest::newRow("extractplanes_formats") << QString("format=yuv444p|yuv422p|yuv420p|yuv410p,extractplanes=v,histeq=strength=0.2:intensity=0.2");
    QTest::newRow("bottom_blend") << QString("split[a][b];[a]field=bottom[a1];[b]field=top,negate[b2];[a1][b2]blend=all_mode=average,histeq=strength=0:intensity=0");
    QTest::newRow("force_original_aspect_ratio") << QString("scale=iw/8:ih/4:force_original_aspect_ratio=decrease,tile=8x4:overlap=8*4-1:init_padding=8*4-1");
    QTest::newRow("histogram_linear") << QString("histogram=level_height=576:levels_mode=linear");
    //QTest::newRow("thistogram") << QString("thistogram=levels_mode=linear"); does not exist in old ffmpeg version
    QTest::newRow("shuffleplanes") << QString("limiter=min=0:max=255:planes=1,shuffleplanes=1-1,histeq=strength=0.2,format=gray,format=yuv444p");
    QTest::newRow("crop_overlap") << QString("format=rgb24|yuv444p,crop=iw:1:0:288:0:1,tile=1x480:overlap=480-1:init_padding=480-1");
    QTest::newRow("crop_mirror") << QString("crop=iw:1:0:288:0:1,waveform=intensity=1:mode=column:mirror=1:components=7:display=overlay:graticule=green:flags=numbers+dots:scale=0");
    QTest::newRow("oscilloscope") << QString("oscilloscope=x=500000/1000000:y=500000/1000000:s=500000/1000000:t=500000/1000000");
    QTest::newRow("geq") << QString("geq=lum=lum(X\\,Y)-lum(X-1\\,Y-0)+128:cb=cb(X\\,Y)-cb(X-0\\,Y-0)+128:cr=cr(X\\,Y)-cr(X-0\\,Y-0)+128,histeq=strength=0");
    QTest::newRow("pixscope") << QString("pixscope=x=20/100:y=20/100:w=8:h=8,format=rgb24");
    QTest::newRow("crop_in_range") << QString("split[a][b];[a]crop=100:100:0:0[a1];[b]scale=iw+1:ih:in_range=tv:out_range=full,scale=iw-1:ih[b1];[b1][a1]overlay");
    QTest::newRow("between_hypot") << QString("format=yuv444p,geq=lum=lum(X\\,Y):cb=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,89\\,182)\\,32\\,128):cr=if(between(hypot(cb(X\\,Y)-128\\,cr(X\\,Y)-128)\\,89\\,182)\\,220\\,128)");
    QTest::newRow("tblend") << QString("tblend=all_mode=difference128,histeq=strength=0.2:intensity=0.2");
    QTest::newRow("signalstats") << QString("format=yuv444p,signalstats=out=tout:c=0x40e0d0");
    QTest::newRow("extractplanes_between") << QString("extractplanes=y,format=rgb24,lutrgb=r=if(between(val\\,235\\,255)\\,64\\,val):g=if(between(val\\,235\\,255)\\,224\\,val):b=if(between(val\\,235\\,255)\\,208\\,val)");
    QTest::newRow("vectorscope_envelope") << QString("vectorscope=i=0.1:mode=3:envelope=0:colorspace=1:graticule=green:flags=name,pad=ih*1.33333:ih:(ow-iw)/2:(oh-ih)/2");
    QTest::newRow("vectorscope_graticule") << QString("split[h][l];[l]vectorscope=i=0.1:mode=3:envelope=0:colorspace=1:graticule=green:flags=name:l=0:h=0.5[l1];[h]vectorscope=i=0.1:mode=3:envelope=0:colorspace=1:graticule=green:flags=name:l=0.5:h=1[h1];[l1][h1]hstack");
    QTest::newRow("lutyuv_drawbox") << QString("split[a][b];            [a]lutyuv=y=val/4,scale=720:576,setsar=1/1,format=yuv444p|yuv444p10le,drawbox=w=120:h=120:x=20:y=20:color=invert:thickness=1[a1];            [b]crop=120:120:20:20,            format=yuv422p|yuv422p10le|yuv420p|yuv411p|yuv444p|yuv444p10le,vectorscope=i=0.1:mode=3:envelope=0:colorspace=601:graticule=green:flags=name,pad=ih*1.33333:ih:(ow-iw)/2:(oh-ih)/2,scale=720:576,setsar=1/1[b1];            [a1][b1]blend=addition");
    QTest::newRow("signalstats_vrep") << QString("format=yuv444p,signalstats=out=vrep:c=0x40e0d0");
    QTest::newRow("waveform_green") << QString("waveform=intensity=0.1:mode=column:mirror=1:c=1:f=0:graticule=green:flags=numbers+dots:scale=0");
    QTest::newRow("lutyuv_drawbox_green") << QString("split[a][b];            [a]lutyuv=y=val/4,scale=720:576,setsar=1/1,format=yuv444p|yuv444p10le,drawbox=w=121:h=121:x=20:y=20:color=invert:thickness=1[a1];            [b]crop=121:121:20:20,            waveform=intensity=0.8:mode=column:mirror=1:c=1:f=0:graticule=green:flags=numbers+dots:scale=0,scale=720:576,setsar=1/1[b1];            [a1][b1]blend=addition");
    QTest::newRow("crop_neighbor") << QString("crop=x=200:y=200:w=120:h=120,scale=720:576:flags=neighbor,histeq=strength=0,setsar=1/1");
    QTest::newRow("pad_negate_edgedetect_overlay") << QString("[0:v]pad=iw*2:ih*2[a];  [1:v]negate[b];  [2:v]hflip[c];  [3:v]edgedetect[d];  [a][b]overlay=w[x];  [x][c]overlay=0:h[y];  [y][d]overlay=w:h[out]");
    //QTest::newRow("waveform_audio_video") << QString("[0:v]waveform[v];[1:a]showwaves=s=640x256[a];[v][a]xstack");
    QTest::newRow("negate_hflip_edgedetect_hstack") << QString("[1:v]negate[a];  [2:v]hflip[b];  [3:v]edgedetect[c];  [0:v][a]hstack=inputs=2[top];  [b][c]hstack=inputs=2[bottom];  [top][bottom]vstack=inputs=2[out]");
    QTest::newRow("crop_black") << QString("crop=in_w-2*150:in_h,pad=980:980:x=0:y=0:color=black");
}

void tst_QAVPlayer::filter()
{
    QFETCH(QString, filter);
    QAVPlayer p;

    QFileInfo file(testData("dv25_pal__411_4-3_2ch_32k_bars_sine.dv"));
    p.setSource(file.absoluteFilePath());

    QSignalSpy spyVideoFilterChanged(&p, &QAVPlayer::filtersChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; });

    p.setFilter(filter);
    p.pause();
    QTRY_VERIFY_WITH_TIMEOUT(frame, 10000);
    QTRY_COMPARE(spyVideoFilterChanged.count(), 1);

    frame = QAVVideoFrame();

    p.play();
    QTRY_VERIFY(frame);
    QCOMPARE(p.filters(), {filter});
    QCOMPARE(spyErrorOccurred.count(), 0);

    QTest::qWait(100);

    QCOMPARE(spyErrorOccurred.count(), 0);
    p.seek(p.duration());
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 10000);
}

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

    Buffer buffer;
    buffer.m_size = file.size();
    buffer.open(QIODevice::ReadWrite);

    QAVPlayer p;
    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.setSource(fileInfo.fileName(), &buffer);
    p.play();

    while(!file.atEnd()) {
        auto bytes = file.read(64 * 1024);
        buffer.write(bytes);
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
    file.open(QFile::ReadOnly);

    BufferSequential buffer;
    buffer.m_size = file.size();
    buffer.open(QIODevice::ReadWrite);

    QAVPlayer p;
    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.setSource(fileInfo.fileName(), &buffer);
    p.play();

    while(!file.atEnd()) {
        auto bytes = file.read(64 * 1024);
        buffer.write(bytes);
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

void tst_QAVPlayer::bsf()
{
    QAVPlayer p;
    QFileInfo file(testData("test.mov"));
    p.setSource(file.absoluteFilePath());

    QSignalSpy spy(&p, &QAVPlayer::bitstreamFilterChanged);

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.setBitstreamFilter("noise");
    p.play();
    QTRY_COMPARE(spy.count(), 1);
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);

    spy.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("noise");
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);

    spy.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("");
    QTRY_COMPARE(spy.count(), 1);
    p.setSource("");
    p.setSource(file.absoluteFilePath());

    spy.clear();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.bitstreamFilter().isEmpty());

    p.setBitstreamFilter("noise");
    p.play();

    QVERIFY(!p.bitstreamFilter().isEmpty());
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);

    spy.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("noise");
    QTRY_VERIFY(framesCount > 0);
    QVERIFY(frame);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::bsfInvalid()
{
    QAVPlayer p;
    QFileInfo file(testData("test.mov"));
    p.setSource(file.absoluteFilePath());

    QSignalSpy spy(&p, &QAVPlayer::bitstreamFilterChanged);
    QSignalSpy spyErrorOccurred(&p, &QAVPlayer::errorOccurred);

    QAVVideoFrame frame;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });

    p.setBitstreamFilter("obey=666");
    p.play();

    QTRY_COMPARE(spy.count(), 1);
    QTRY_VERIFY(spyErrorOccurred.count() > 0);

    QCOMPARE(framesCount, 0);
    QVERIFY(!frame);

    spy.clear();
    spyErrorOccurred.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("");
    QTRY_COMPARE(spy.count(), 1);
    p.setSource("");
    p.setSource(file.absoluteFilePath());

    spy.clear();
    spyErrorOccurred.clear();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QVERIFY(p.bitstreamFilter().isEmpty());

    p.setBitstreamFilter("makeluv=69");
    p.play();

    QVERIFY(!p.bitstreamFilter().isEmpty());
    QTRY_COMPARE(spyErrorOccurred.count(), 1);
    QCOMPARE(framesCount, 0);
    QVERIFY(!frame);

    spyErrorOccurred.clear();

    p.setBitstreamFilter("");
    p.play();
    QTRY_VERIFY(frame);
    QVERIFY(framesCount > 0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spyErrorOccurred.count(), 0);

    p.setBitstreamFilter("not=war");
    p.play();

    QTRY_COMPARE(spyErrorOccurred.count(), 1);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::InvalidMedia);

    spy.clear();
    spyErrorOccurred.clear();
    framesCount = 0;
    frame = QAVVideoFrame();

    p.setBitstreamFilter("noise");
    p.play();

    QTRY_VERIFY(frame);
    QVERIFY(framesCount > 0);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QCOMPARE(spyErrorOccurred.count(), 0);
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
        md2 = frame.map();
    });
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        md3 = frame.map();
        md3 = frame.map();
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
    p.setSource(file.absoluteFilePath());
    p.setFilter("[0:v]split=2[in1][in2];[in1]boxblur[out1];[in2]negate[out2]");
    QAVVideoFrame videoFrame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        videoFrame = frame;
    });

    p.play();
    QTRY_VERIFY_WITH_TIMEOUT(videoFrame, 30000);
    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
}

void tst_QAVPlayer::filterName()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;
    QFileInfo file(testData("small.mp4"));
    p.setSource(file.absoluteFilePath());
    p.setFilter("scale=iw/2:-1");
    QSet<QString> set;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        qDebug() << frame.pts() << frame.filterName();
        if (!frame.filterName().isEmpty())
            set.insert(frame.filterName());
    });

    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(set.size(), 1, 30000);
    QVERIFY(set.contains("0:0"));
    set.clear();
    p.setFilter("[0:v]split=2[in1][in2];[in1]boxblur[out1];[in2]negate[out2]");
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 2, 30000);
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    set.clear();
    p.setFilter("[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]");
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 3, 30000);
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    set.clear();
    p.setFilters({
            "scale=iw/2:-1[scale]",
            "negate[negate]",
            "[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]"
        });
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 5, 30000);
    QTRY_VERIFY(set.contains("scale"));
    QTRY_VERIFY(set.contains("negate"));
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
}

void tst_QAVPlayer::filterNameStep()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;
    QFileInfo file(testData("small.mp4"));
    p.setSource(file.absoluteFilePath());
    p.setFilter("[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]");
    QSet<QString> set;
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        qDebug() << frame.pts() << frame.filterName();
        if (!frame.filterName().isEmpty())
            set.insert(frame.filterName());
        ++framesCount;
    });

    p.pause();
    QTRY_VERIFY_WITH_TIMEOUT(set.size() == 3, 5000);
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    QCOMPARE(framesCount, 3);

    set.clear();
    framesCount = 0;
    p.setFilters({
            "scale=iw/2:-1[scale]",
            "negate[negate]",
            "[0:v]split=3[in1][in2][in3];[in1]boxblur[out1];[in2]negate[out2];[in3]scale=iw/2:-1[out3]"
        });

    p.stepForward();
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 5, 30000);
    QTRY_VERIFY(set.contains("scale"));
    QTRY_VERIFY(set.contains("negate"));
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    QVERIFY(framesCount >= 5);

    set.clear();
    framesCount = 0;

    p.stepBackward();
    QTRY_VERIFY_WITH_TIMEOUT(set.size() >= 5, 30000);
    QTRY_VERIFY(set.contains("scale"));
    QTRY_VERIFY(set.contains("negate"));
    QTRY_VERIFY(set.contains("out1"));
    QTRY_VERIFY(set.contains("out2"));
    QTRY_VERIFY(set.contains("out3"));
    QCOMPARE(framesCount, 5);

    set.clear();
    framesCount = 0;

    p.setFilter("");
    p.stepForward();
    QTRY_COMPARE(framesCount, 1);
    QVERIFY(set.isEmpty());
    set.clear();
    framesCount = 0;

    p.stepForward();
    QTRY_COMPARE(framesCount, 1);
    QVERIFY(set.isEmpty());
}

void tst_QAVPlayer::audioVideoFilter()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;
    QFileInfo file(testData("test.mkv"));
    p.setSource(file.absoluteFilePath());
    p.setFilter("ahistogram=dmode=separate:rheight=0:s=360x1:r=32,transpose=2,tile=layout=512x1,format=rgb24 [panel_4]");

    int framesCount = 0;
    QAVVideoFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        frame = f;
        ++framesCount;
    }, Qt::DirectConnection);

    p.setSynced(false);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QCOMPARE(framesCount, 1);
    QCOMPARE(frame.filterName(), "panel_4");
}

void tst_QAVPlayer::audioFilterVideoFrames()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;
    QFileInfo file(testData("test.mkv"));
    p.setSource(file.absoluteFilePath());
    p.setFilter("aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4,aphasemeter=video=0,ebur128=metadata=1,aformat=sample_fmts=flt|fltp");

    int videoFramesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &) {
        ++videoFramesCount;
    }, Qt::DirectConnection);

    int audioFramesCount = 0;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &) {
        ++audioFramesCount;
    }, Qt::DirectConnection);


    p.setSynced(false);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QCOMPARE(videoFramesCount, 0);
    QVERIFY(audioFramesCount > 0);
}

void tst_QAVPlayer::multipleFilters()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;
    QFileInfo file(testData("test.mkv"));
    p.setSource(file.absoluteFilePath());
    QList<QString> filters = {
        "signalstats=stat=tout+vrep+brng [stats]",
        "aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4,aphasemeter=video=0,ebur128=metadata=1,aformat=sample_fmts=flt|fltp",
        "scale=72:72,format=rgb24 [thumbnails]",
        "scale=iw/4:ih/4,format=gray,convolution=0m='0 1 0 1 -4 1 0 1 0':0bias=128,split[a][b];[a]scale=iw:1[a1];[a1][b]scale2ref[a2][b];[b][a2]lut2=c0=((x-y)*(x-y))/2,scale=iw:1,transpose=2,tile=layout=512x1,setsar=1/1,format=rgb24 [panel_0]",
        "scale,format=rgb24,crop=1:ih:iw/2:0,tile=layout=512x1,setsar=1/1 [panel_1]",
        "scale,format=rgb24,transpose=2,crop=1:ih:iw/2:0,tile=layout=512x1,setsar=1/1 [panel_2]",
        "aformat=channel_layouts=stereo:sample_fmts=flt|fltp,ahistogram=dmode=separate:rheight=0:s=360x1:r=32,transpose=2,tile=layout=512x1,format=rgb24 [panel_3]",
        "aformat=channel_layouts=stereo:sample_fmts=flt|fltp,showwaves=mode=p2p:split_channels=1:size=512x360:scale=lin:draw=full:rate=32/512,format=rgb24 [panel_4]",
    };

    QMap<QString, int> framesCount;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) {
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    p.setSynced(false);
    p.setFilters(filters);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QVERIFY(framesCount.contains("stats"));
    QCOMPARE(framesCount["stats"], 250);
    QVERIFY(framesCount.contains("1:0"));
    QCOMPARE(framesCount["1:0"], 101);
    QVERIFY(framesCount.contains("thumbnails"));
    QCOMPARE(framesCount["thumbnails"], 250);
    QVERIFY(framesCount.contains("panel_0"));
    QCOMPARE(framesCount["panel_0"], 1);
    QVERIFY(framesCount.contains("panel_1"));
    QCOMPARE(framesCount["panel_1"], 1);
    QVERIFY(framesCount.contains("panel_2"));
    QCOMPARE(framesCount["panel_2"], 1);
    QVERIFY(framesCount.contains("panel_3"));
    QCOMPARE(framesCount["panel_3"], 1);
    QVERIFY(framesCount.contains("panel_4"));
    QCOMPARE(framesCount["panel_4"], 1);
}

void tst_QAVPlayer::multipleAudioVideoFilters()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;
    QFileInfo file(testData("test_5beeps.mkv"));
    p.setSource(file.absoluteFilePath());
    QList<QString> filters = {
        "signalstats=stat=tout+vrep+brng [stats]",
        "aformat=sample_fmts=flt|fltp,astats=metadata=1:reset=1:length=0.4,aphasemeter=video=0,ebur128=metadata=1,aformat=sample_fmts=flt|fltp [audio]",
    };

    QMap<QString, int> framesCount;
    QAVVideoFrame videoFrame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) {
        videoFrame = f;
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    QAVAudioFrame audioFrame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) {
        audioFrame = f;
        ++framesCount[f.filterName()];
    }, Qt::DirectConnection);

    p.setSynced(false);
    p.setFilters(filters);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QVERIFY(framesCount.contains("stats"));
    QCOMPARE(framesCount["stats"], 125);
    QCOMPARE(videoFrame.pts(), 4.963);
    QVERIFY(framesCount.contains("audio"));
    QCOMPARE(framesCount["audio"], 51);
    QVERIFY(audioFrame.pts() < 5.5);
}

void tst_QAVPlayer::inputFormat()
{
    QAVPlayer p;
    QSignalSpy spy(&p, &QAVPlayer::inputFormatChanged);
    QCOMPARE(p.inputFormat(), "");
    p.setInputFormat("v4l2");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.inputFormat(), "v4l2");
}

void tst_QAVPlayer::inputVideoCodec()
{
    QAVPlayer p;
    QFileInfo file(testData("small.mp4"));
    QSignalSpy spy(&p, &QAVPlayer::inputVideoCodecChanged);
    p.setSource(file.absoluteFilePath());
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &) { ++framesCount; });

    QCOMPARE(p.inputVideoCodec(), "");
    p.setInputVideoCodec("h264");
    QCOMPARE(spy.count(), 1);
    QCOMPARE(p.inputVideoCodec(), "h264");
    QVERIFY(!QAVPlayer::supportedVideoCodecs().isEmpty());
    if (!QAVPlayer::supportedVideoCodecs().contains("h264"))
        return;

    p.setSynced(false);
    p.play();
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QTRY_COMPARE(framesCount, 166);
}

void tst_QAVPlayer::flushFilters()
{
    qputenv("QT_AVPLAYER_NO_HWDEVICE", "1");
    QAVPlayer p;
    QFileInfo file(testData("BAVC1010958_DV000107.dv"));
    p.setSource(file.absoluteFilePath());
    p.setFilter("scale,format=rgb32,crop=1:ih:iw/2:0,tile=layout=512x1,setsar=1/1 [panel_0]");
    int framesCount = 0;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &) {
        ++framesCount;
    });
    p.play();
    p.setSynced(false);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QTRY_COMPARE(framesCount, 1);
}

void tst_QAVPlayer::multipleAudioStreams()
{
    QAVPlayer p;

    QFileInfo file(testData("guido.mp4"));

    QSignalSpy spy(&p, &QAVPlayer::audioStreamsChanged);

    QSet<int> streams;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { streams.insert(f.stream().index()); });

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
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frameAudio = f; streamsAudio.insert(f.stream().index()); });
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

void tst_QAVPlayer::flushCodecs()
{
    QAVPlayer p;
    QFileInfo file(testData("DHC0413_CreaseOrNot.mp4"));
    int framesCount = 0;
    QAVFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; });
    qint64 pos = 0;
    QObject::connect(&p, &QAVPlayer::played, &p, [&](qint64 p) { pos = p; });

    p.setSource(file.absoluteFilePath());
    p.setSynced(false);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QVERIFY(frame);
    QVERIFY(frame.stream());
    QCOMPARE(frame.stream().framesCount(), 309);
    if (pos > 0) {
        qDebug() << "Played from" << pos;
        return;
    }
    QTRY_COMPARE(framesCount, 309);

    frame = {};
    framesCount = 0;
    p.setSynced(true);
    QVERIFY(p.isSynced());
    p.setSpeed(2);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::LoadedMedia);
    QTRY_COMPARE_WITH_TIMEOUT(p.mediaStatus(), QAVPlayer::EndOfMedia, 15000);
    QVERIFY(frame);
    QVERIFY(frame.stream());
    QCOMPARE(frame.stream().framesCount(), 309);
    QTRY_COMPARE(framesCount, 309);
}

void tst_QAVPlayer::multiFilterInputs_data()
{
    QTest::addColumn<QString>("filter");

    QTest::newRow("vstack") << QString("sws_flags=neighbor;format=yuv444p,scale[b];showvolume=w=320:h=40:f=0.95:dm=1[a];[a][b]vstack");
    QTest::newRow("xstack") << QString("[0:a:0]abitscope,scale=320x240[z2];[0:v:0]scale=320x240[b];[z2][b]xstack");
}

void tst_QAVPlayer::multiFilterInputs()
{
    QFETCH(QString, filter);
    QAVPlayer p;
    QFileInfo file(testData("av_sample.mkv"));
    int framesCount = 0;
    QAVFrame frame;
    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &f) { frame = f; ++framesCount; }, Qt::DirectConnection);

    p.setSource(file.absoluteFilePath());
    p.setSynced(false);
    p.setFilter(filter);
    p.play();

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QVERIFY(frame);
    QVERIFY(frame.stream());
    QCOMPARE(framesCount, 250);
    auto s = p.currentVideoStreams().first();
    QCOMPARE(framesCount, s.framesCount());
    QCOMPARE(framesCount, p.progress(s).framesCount());
    QCOMPARE(framesCount, p.progress(s).expectedFramesCount());
    QVERIFY(p.progress(s).pts() > 0);
    QVERIFY(p.progress(s).fps() > 0.0);
    QVERIFY(p.progress(s).frameRate() > 0.0);
    QVERIFY(p.progress(s).expectedFrameRate() > 0.0);
}

QTEST_MAIN(tst_QAVPlayer)
#include "tst_qavplayer.moc"
