/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#pragma once

#include <QObject>
#include <QString>
#include <QtCore/QtGlobal>

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
    void audioPositionWithCover();
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
    void stepForward();
    void stepBackward();
    void availableAudioStreams();
#ifdef QT_AVPLAYER_MULTIMEDIA
    void cast2QVideoFrame_data();
    void cast2QVideoFrame();
    void audioOutput();
    void multiPlayers();
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
    void streamMetadataRotate();
    void switchingSource();
    void outputFile();
    void muxerFilters();
    void muxerMultiSourceFrames();
    void framesAfterPlayerDestroyed();
    void scaleHW();
    void muxerScaleHW_data();
    void muxerScaleHW();
    void muxerScaleHWSplit();
    void muxerScale_data();
    void muxerScale();
};
