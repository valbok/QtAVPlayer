/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include <QtAVPlayer>
#include <private/qdeclarativevideooutput_p.h>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlEngine>
#include <QGuiApplication>

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
class Source : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractVideoSurface *videoSurface READ videoSurface WRITE setVideoSurface)
public:
    explicit Source(QObject *parent = 0) : QObject(parent) { }
    virtual ~Source() { }

    QAbstractVideoSurface* videoSurface() const { return m_surface; }
    void setVideoSurface(QAbstractVideoSurface *surface)
    {
        m_surface = surface;
    }

    QAbstractVideoSurface *m_surface = nullptr;
};
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQuickView viewer;
    viewer.setSource(QUrl("qrc:///main.qml"));
    viewer.setResizeMode(QQuickView::SizeRootObjectToView);
    QObject::connect(viewer.engine(), SIGNAL(quit()), &viewer, SLOT(close()));

    QQuickItem *rootObject = viewer.rootObject();
    auto vo = rootObject->findChild<QDeclarativeVideoOutput *>("videoOutput");

    QAVPlayer p;
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    Source src;
    vo->setSource(&src);
    if (src.m_surface)
        p.vo(src.m_surface);
#else
    p.vo(vo->videoSurface());
#endif

    /*QAVAudioOutput audioOutput;
    p.ao([&audioOutput](const QAudioBuffer &buf) {  audioOutput.play(buf); });
    p.vo([vo](const QVideoFrame &videoFrame) {
        if (!vo->videoSurface()->isActive())
            vo->videoSurface()->start({videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType()});
        if (vo->videoSurface()->isActive())
            vo->videoSurface()->present(videoFrame);
    });*/
    p.setSource(QUrl("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4"));
    p.play();

    QObject::connect(&p, &QAVPlayer::stateChanged, [&](auto s) {
        qDebug() << "stateChanged" << s << p.mediaStatus();
    });
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, [&](auto s){
        qDebug() << "mediaStatusChanged"<< s << p.state();
    });
    QObject::connect(&p, &QAVPlayer::durationChanged, [&](auto d) {
        qDebug() << "durationChanged" << d;
    });

    viewer.setMinimumSize(QSize(300, 360));
    viewer.resize(1960, 1086);
    viewer.show();

    return app.exec();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#include "main.moc"
#endif