/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include <QtAVPlayer/qavplayer.h>
#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudioframe.h>
#include <QGuiApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QAVPlayer p;
    QObject::connect(&p, &QAVPlayer::audioFrame, [&](const QAVAudioFrame &frame) { qDebug() << "audioFrame" << bool(frame); });
    QObject::connect(&p, &QAVPlayer::videoFrame, [&](const QAVVideoFrame &frame) { qDebug() << "videoFrame" << bool(frame); });
    p.setSource(QLatin1String("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"));
    p.play();

    QObject::connect(&p, &QAVPlayer::stateChanged, [&](auto s) { qDebug() << "stateChanged" << s << p.mediaStatus(); });
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, [&](auto s){ qDebug() << "mediaStatusChanged"<< s << p.state(); });
    QObject::connect(&p, &QAVPlayer::durationChanged, [&](auto d) { qDebug() << "durationChanged" << d; });

    return app.exec();
}

