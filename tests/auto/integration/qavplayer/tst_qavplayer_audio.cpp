/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "tst_qavplayer.h"
#include "qavplayer.h"

#include <QtTest/QtTest>

void tst_QAVPlayer::playAudioOutput()
{
    QAVPlayer p;

    QFileInfo file(testData("test.wav"));
    p.setSource(file.absoluteFilePath());

    QAVAudioFrame frame;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&](const QAVAudioFrame &f) { frame = f; }, Qt::DirectConnection);
    p.play();

    QTRY_VERIFY(p.position() != 0);
    QTRY_VERIFY(frame);
    QCOMPARE(frame.format().sampleFormat(), QAVAudioFormat::Int16);

    QTRY_COMPARE(p.mediaStatus(), QAVPlayer::EndOfMedia);
    QTRY_COMPARE(p.state(), QAVPlayer::StoppedState);
    QCOMPARE(p.position(), p.duration());
}
