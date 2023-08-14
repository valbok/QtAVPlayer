/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFVIDEORAME_H
#define QAVFVIDEORAME_H

#include <QtAVPlayer/qavframe.h>
#include <QVariant>
#ifdef QT_AVPLAYER_MULTIMEDIA
#include <QVideoFrame>
#endif

extern "C" {
#include <libavutil/frame.h>
}

QT_BEGIN_NAMESPACE

class QAVVideoFramePrivate;
class QAVCodec;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QRhi;
#endif
class QAVVideoFrame : public QAVFrame
{
public:
    enum HandleType
    {
        NoHandle,
        GLTextureHandle,
        MTLTextureHandle,
        D3D11Texture2DHandle
    };

    QAVVideoFrame();
    QAVVideoFrame(const QAVFrame &other);
    QAVVideoFrame(const QAVVideoFrame &other);
    QAVVideoFrame(const QSize &size, AVPixelFormat fmt);

    QAVVideoFrame &operator=(const QAVFrame &other);
    QAVVideoFrame &operator=(const QAVVideoFrame &other);

    QSize size() const;

    struct MapData
    {
        int size = 0;
        int bytesPerLine[4] = {0};
        uchar *data[4] = {nullptr};
        AVPixelFormat format = AV_PIX_FMT_NONE;
    };

    MapData map() const;
    HandleType handleType() const;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QVariant handle(QRhi *rhi = nullptr) const;
#else
    QVariant handle() const;
#endif
    AVPixelFormat format() const;
    QString formatName() const;
    QAVVideoFrame convertTo(AVPixelFormat fmt) const;
#ifdef QT_AVPLAYER_MULTIMEDIA
    operator QVideoFrame() const;
#endif

protected:
    Q_DECLARE_PRIVATE(QAVVideoFrame)
};

Q_DECLARE_METATYPE(QAVVideoFrame)
Q_DECLARE_METATYPE(AVPixelFormat)

QT_END_NAMESPACE

#endif
