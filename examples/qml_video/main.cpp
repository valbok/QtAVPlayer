/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include <QtAVPlayer/qavplayer.h>
#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudiooutput.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAbstractVideoSurface>
#include <private/qdeclarativevideooutput_p.h>
#else
#include <QVideoSink>
#include <QtMultimediaQuick/private/qquickvideooutput_p.h>
#endif

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#include <QtQml/QQmlEngine>
#include <QGuiApplication>
#include <QDebug>

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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using VideoOutput = QDeclarativeVideoOutput;
#else
    using VideoOutput = QQuickVideoOutput;
#endif

    auto vo = rootObject->findChild<VideoOutput *>("videoOutput");

    QAVAudioOutput audioOutput;
    QAVPlayer p;

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    Source src;
    vo->setSource(&src);
    auto videoSurface = src.m_surface;
#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    auto videoSurface = vo->videoSurface();
    // Make sure that render geometry has been updated after a frame
    QObject::connect(vo, &QDeclarativeVideoOutput::sourceRectChanged, &p, [&] {
        vo->update();
    });
#else
    auto videoSurface = vo->videoSink();
#endif

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&videoSurface](const QAVVideoFrame &frame) {
        QVideoFrame videoFrame = frame;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (!videoSurface->isActive())
            videoSurface->start({videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType()});
        if (videoSurface->isActive())
            videoSurface->present(videoFrame);
#else
        videoSurface->setVideoFrame(videoFrame);
#endif
    });

    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&audioOutput](const QAVAudioFrame &frame) { audioOutput.play(frame); });
    QString file = argc > 1 ? QLatin1String(argv[1]) : QLatin1String("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");

    QObject::connect(&p, &QAVPlayer::stateChanged, [&](auto s) { qDebug() << "stateChanged" << s << p.mediaStatus(); });
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, [&](auto status) {
        qDebug() << "mediaStatusChanged"<< status << p.state();
        if (status == QAVPlayer::LoadedMedia) {
            qDebug() << "Video streams:" << p.videoStreamsCount();
            qDebug() << "Audio streams:" << p.audioStreamsCount();
        }
    });
    QObject::connect(&p, &QAVPlayer::durationChanged, [&](auto d) { qDebug() << "durationChanged" << d; });

    p.setSource(file);
    p.play();

    viewer.setMinimumSize(QSize(300, 360));
    viewer.resize(1960, 1086);
    viewer.show();

    return app.exec();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#include "main.moc"
#endif
