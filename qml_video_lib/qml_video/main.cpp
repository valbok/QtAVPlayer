/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"
#include <private/qdeclarativevideooutput_p.h>
#include <QTimer>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlEngine>
#include <QGuiApplication>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickView viewer;
    viewer.setSource(QUrl("qrc:///main.qml"));
    viewer.setResizeMode(QQuickView::SizeRootObjectToView);
    QObject::connect(viewer.engine(), SIGNAL(quit()), &viewer, SLOT(close()));

    QQuickItem *rootObject = viewer.rootObject();
    auto vo = rootObject->findChild<QDeclarativeVideoOutput *>("videoOutput");

    viewer.setMinimumSize(QSize(300, 360));
    viewer.resize(1960, 1086);
    viewer.show();

    QAVPlayer p;
    p.setVideoSurface(vo->videoSurface());
    p.setSource(QUrl("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"));
    p.play();

    int i = 0;
    QTimer t;
    t.setInterval(1000);
    QObject::connect(&t, &QTimer::timeout, [&]() {
        p.seek(rand() % 10000);
        ++i;
        if(i == 3) {
            p.pause();
        }
    });
    t.start();

    QObject::connect(&p, &QAVPlayer::stateChanged, [&](auto s) {
        qDebug() << "stateChanged" << s << p.mediaStatus();
    });
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, [&](auto s){
        qDebug() << "mediaStatusChanged"<< s << p.state();
    });
    QObject::connect(&p, &QAVPlayer::durationChanged, [&](auto d) {
        qDebug() << "durationChanged" << d;
    });

    return app.exec();
}

