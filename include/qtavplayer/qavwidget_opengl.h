/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVWIDGET_OPENGL_H
#define QAVWIDGET_OPENGL_H

#include "qtavplayer/qavvideoframe.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_0>

#include <memory>

QT_BEGIN_NAMESPACE

class QAVWidget_OpenGLPrivate;
class QAVWidget_OpenGL : public QOpenGLWidget, public QOpenGLFunctions_3_0
{
    Q_OBJECT
public:
    QAVWidget_OpenGL(QWidget *parent = nullptr);
    ~QAVWidget_OpenGL();

    void setVideoFrame(const QAVVideoFrame &frame);

private:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;

    std::unique_ptr<QAVWidget_OpenGLPrivate> d_ptr;
    Q_DISABLE_COPY(QAVWidget_OpenGL)
    Q_DECLARE_PRIVATE(QAVWidget_OpenGL)
};

QT_END_NAMESPACE

#endif  // QAVGLWIDGET_H
