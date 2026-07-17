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
#include <QGridLayout>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget mainWidget;
    QGridLayout layout;
    mainWidget.setLayout(&layout);

    int r = 0;
    int c = 0;
    for (int i = 1; i < argc; ++i, ++c) {
        if (c > 2) {
            ++r;
            c = 0;
        }

        auto p = new QAVPlayer(&mainWidget);
        p->setSource(QString::fromUtf8(argv[i]));
        auto w = new QAVWidget_OpenGL(&mainWidget);
        layout.addWidget(w, r, c);
        QObject::connect(p, &QAVPlayer::videoFrame, w, [w](const QAVVideoFrame &frame) {
            w->setVideoFrame(frame);
        }, Qt::DirectConnection);

        if (argc == 2) {
            auto audioOutput = new QAVAudioOutput(&mainWidget);
            QObject::connect(p, &QAVPlayer::audioFrame, audioOutput, [audioOutput](const QAVAudioFrame &frame) {
                audioOutput->play(frame);
            }, Qt::DirectConnection);
        }

        p->play();
    }

    mainWidget.resize(1280, 720);
    mainWidget.show();
    return app.exec();
}
