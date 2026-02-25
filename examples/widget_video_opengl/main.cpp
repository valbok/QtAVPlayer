/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include <QtAVPlayer/qavwidget_opengl.h>
#include <QtAVPlayer/qavplayer.h>
#include <QtAVPlayer/qavaudiooutput.h>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QAVWidget_OpenGL w;
    w.resize(1000, 800);
    w.show();

    QAVPlayer p;
    QString file = argc > 1 ? QString::fromUtf8(argv[1]) : QString::fromLatin1("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4");
    p.setSource(file);

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        w.setVideoFrame(frame);
    }, Qt::DirectConnection);

    QAVAudioOutput audioOutput;
    QObject::connect(&p, &QAVPlayer::audioFrame, &audioOutput, [&audioOutput](const QAVAudioFrame &frame) {
        audioOutput.play(frame);
    }, Qt::DirectConnection);

    p.play();

    return app.exec();
}
