/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include <QtAVPlayer/qavplayer.h>
#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudiooutput.h>
#include <QVideoWidget>
#include <QApplication>
#include <QDebug>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <QMediaService>
#include <QMediaObject>
#include <QVideoRendererControl>

class VideoRenderer : public QVideoRendererControl
{
public:
    QAbstractVideoSurface *surface() const override
    {
        return m_surface;
    }

    void setSurface(QAbstractVideoSurface *surface) override
    {
        m_surface = surface;
    }

    QAbstractVideoSurface *m_surface = nullptr;
};

class MediaService : public QMediaService
{
public:
    MediaService(VideoRenderer *vr, QObject* parent = nullptr)
        : QMediaService(parent)
        , m_renderer(vr)
    {
    }

    QMediaControl* requestControl(const char *name) override
    {
        if (qstrcmp(name, QVideoRendererControl_iid) == 0)
            return m_renderer;

        return nullptr;
    }

    void releaseControl(QMediaControl *) override
    {
    }

    VideoRenderer *m_renderer = nullptr;
};

class MediaObject : public QMediaObject
{
public:
    explicit MediaObject(VideoRenderer *vr, QObject* parent = nullptr)
        : QMediaObject(parent, new MediaService(vr, parent))
    {
    }
};

class VideoWidget : public QVideoWidget
{
public:
    bool setMediaObject(QMediaObject *object) override
    {
        return QVideoWidget::setMediaObject(object);
    }
};
#else
#include <QVideoSink>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    VideoRenderer vr;

    VideoWidget w;
    w.show();

    MediaObject mo(&vr);
    w.setMediaObject(&mo);
#else
    QVideoWidget w;
    w.show();
#endif

    QAVPlayer p;
    QString file = argc > 1 ? QLatin1String(argv[1]) : QLatin1String("http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4");
    p.setSource(file);
    p.play();

    QAVAudioOutput audioOutput;
    QObject::connect(&p, &QAVPlayer::audioFrame, &p, [&audioOutput](const QAVAudioFrame &frame) { audioOutput.play(frame); });

    QObject::connect(&p, &QAVPlayer::videoFrame, &p, [&](const QAVVideoFrame &frame) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (vr.m_surface == nullptr)
            return;
#endif
        QVideoFrame videoFrame = frame.convertTo(AV_PIX_FMT_RGB32);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        if (!vr.m_surface->isActive() || vr.m_surface->surfaceFormat().frameSize() != videoFrame.size()) {
            QVideoSurfaceFormat f(videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType());
            vr.m_surface->start(f);
        }
        if (vr.m_surface->isActive())
            vr.m_surface->present(videoFrame);
#else
        w.videoSink()->setVideoFrame(videoFrame);
#endif
    });

    return app.exec();
}

