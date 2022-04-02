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
#include <QElapsedTimer>

extern "C" {
#include <libavcodec/avcodec.h>
}
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

    QElapsedTimer frameElapsed;
    frameElapsed.start();
    int frameCount = 0;

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
        const int fps = frameCount++ * 1000 / frameElapsed.elapsed();
        rootObject->setProperty("frame_fps", fps);

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
    QString file = argc > 1 ? QString::fromUtf8(argv[1]) : QLatin1String("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");

    QObject::connect(&p, &QAVPlayer::stateChanged, [&](auto s) { qDebug() << "stateChanged" << s << p.mediaStatus(); });
    QObject::connect(&p, &QAVPlayer::mediaStatusChanged, [&](auto status) {
        qDebug() << "mediaStatusChanged"<< status << p.state();
        if (status == QAVPlayer::LoadedMedia) {
            auto videoStreams = p.videoStreams();
            auto videoStream = p.videoStream();
            qDebug() << "Video streams:" << videoStreams.size();
            for (auto &s : p.videoStreams())
                qDebug() << "[" << s.index() << "]" << s.metadata() << (s.index() == videoStream.index() ? "---current" : "");

            auto audioStreams = p.audioStreams();
            auto audioStream = p.audioStream();
            qDebug() << "Audio streams:" << audioStreams.size();
            for (auto &s : audioStreams)
                qDebug() << "[" << s.index() << "]" << s.metadata() << (s.index() == audioStream.index() ? "---current" : "");

            auto subtitleStreams = p.subtitleStreams();
            qDebug() << "Subtitle streams:" << subtitleStreams.size();
            for (auto &s : subtitleStreams) {
                if (s.metadata()["language"] == "eng") {
                    p.setSubtitleStream(s);
                    break;
                }
            }

            auto subtitleStream = p.subtitleStream();
            for (auto &s : subtitleStreams) {
                qDebug() << "[" << s.index() << "]" << s.metadata() << (s.index() == subtitleStream.index() ? "---current" : "");
            }
        }
    });
    QObject::connect(&p, &QAVPlayer::durationChanged, [&](auto d) { qDebug() << "durationChanged" << d; });

    QObject::connect(&p, &QAVPlayer::subtitleFrame, &p, [](const QAVSubtitleFrame &frame) {
        for (unsigned i = 0; i < frame.subtitle()->num_rects; ++i) {
            if (frame.subtitle()->rects[i]->type == SUBTITLE_TEXT)
                qDebug()<<"text:"<<frame.subtitle()->rects[i]->text;
            else
                qDebug()<<"ass:"<<frame.subtitle()->rects[i]->ass;
        }
    });
    p.setSource(file);
    p.play();

    viewer.setMinimumSize(QSize(300, 360));
    viewer.resize(1960, 1086);
    viewer.show();

    QElapsedTimer qmlElapsed;
    qmlElapsed.start();
    int qmlCount = 0;

    QObject::connect(&viewer, &QQuickView::afterRendering, &viewer, [&] {
        const int fps = qmlCount++ * 1000 / qmlElapsed.elapsed();
        rootObject->setProperty("qml_fps", fps);
    });

    return app.exec();
}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
#include "main.moc"
#endif
