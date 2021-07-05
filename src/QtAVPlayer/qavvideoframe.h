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

QT_BEGIN_NAMESPACE

class QAVVideoFramePrivate;
class QAVCodec;
class Q_AVPLAYER_EXPORT QAVVideoFrame : public QAVFrame
{
public:
    enum HandleType
    {
        NoHandle,
        GLTextureHandle,
        MTLTextureHandle
    };

    QAVVideoFrame(QObject *parent = nullptr);
    QAVVideoFrame(const QAVFrame &other, QObject *parent = nullptr);
    QAVVideoFrame(const QAVVideoFrame &other, QObject *parent = nullptr);

    QAVVideoFrame &operator=(const QAVFrame &other);
    QAVVideoFrame &operator=(const QAVVideoFrame &other);

    QSize size() const;

    struct MapData
    {
        int size = 0;
        int bytesPerLine[4] = {0};
        uchar *data[4] = {nullptr};
    };

    MapData map() const;
    HandleType handleType() const;
    QVariant handle() const;

protected:
    Q_DECLARE_PRIVATE(QAVVideoFrame)
};

Q_DECLARE_METATYPE(QAVVideoFrame)

QT_END_NAMESPACE

#endif
