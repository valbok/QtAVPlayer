/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVASSRENDERER_H
#define QAVASSRENDERER_H

#include <QtAVPlayer/qavstream.h>
#include <QtAVPlayer/qavframe.h>
#include <QtAVPlayer/qavsubtitleframe.h>
#include <QImage>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVASSRendererPrivate;
class QAVASSRenderer
{
public:
    QAVASSRenderer();
    ~QAVASSRenderer();

    int load(const QAVStream &stream);
    void unload();

    QImage toImage(const QAVSubtitleFrame &frame, int width, int height);
    // Flushes the buffer on seek to handle pts change
    void flush();

private:
    Q_DISABLE_COPY(QAVASSRenderer)
    Q_DECLARE_PRIVATE(QAVASSRenderer)
    std::unique_ptr<QAVASSRendererPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
