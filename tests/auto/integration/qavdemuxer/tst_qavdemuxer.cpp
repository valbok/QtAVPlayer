/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qavdemuxer_p.h"
#include "private/qavaudioframe_p.h"
#include "private/qavvideoframe_p.h"
#include "private/qavvideocodec_p.h"
#include "private/qavaudiocodec_p.h"

#include <QtMultimedia/private/qtmultimedia-config_p.h>
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
    QVideoFrame vvf = vf;
    QVERIFY(!vvf.isValid());
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

    QFileInfo file(QLatin1String("../testdata/test.wav"));

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

        auto format = af.codec()->audioFormat();
        QVERIFY(format.isValid());
        auto data = af.data(format);
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

    QFileInfo file(QLatin1String("../testdata/colors.mp4"));

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

            QVideoFrame vvf = vf;
            QVERIFY(vvf.isValid());
            QCOMPARE(vvf.size(), vf.size());
            QVERIFY(vvf.pixelFormat() != QVideoFrame::Format_Invalid);
        }
    }

    QCOMPARE(d.eof(), true);
}

QTEST_MAIN(tst_QAVDemuxer)
#include "tst_qavdemuxer.moc"
