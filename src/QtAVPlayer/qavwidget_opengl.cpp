/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavwidget_opengl.h"

#include <QOpenGLShaderProgram>
#include <QMutexLocker>
#include <qmath.h>
#include <QDebug>

static const char *qt_glsl_vertexShaderProgram =
        "attribute highp vec4 vertexCoordArray;\n"
        "attribute highp vec2 textureCoordArray;\n"
        "uniform highp mat4 positionMatrix;\n"
        "uniform highp float planeWidth1;\n"
        "uniform highp float planeWidth2;\n"
        "uniform highp float planeWidth3;\n"
        "varying highp vec2 plane1TexCoord;\n"
        "varying highp vec2 plane2TexCoord;\n"
        "varying highp vec2 plane3TexCoord;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "   plane1TexCoord = textureCoordArray * vec2(planeWidth1, 1);\n"
        "   plane2TexCoord = textureCoordArray * vec2(planeWidth2, 1);\n"
        "   plane3TexCoord = textureCoordArray * vec2(planeWidth3, 1);\n"
        "   gl_Position = positionMatrix * vertexCoordArray;\n"
        "   textureCoord = textureCoordArray;\n"
        "}\n";

static const char *qt_glsl_rgbShaderProgram =
        "uniform sampler2D tex1;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "   highp vec4 color = vec4(texture2D(tex1, textureCoord.st).rgb, 1.0);\n"
        "   gl_FragColor = vec4(color.rgb, texture2D(tex1, textureCoord.st).a);\n"
        "}\n";

static const char *qt_glsl_bgrShaderProgram =
        "uniform sampler2D tex1;\n"
        "varying highp vec2 textureCoord;\n"
        "void main(void)\n"
        "{\n"
        "   highp vec4 color = vec4(texture2D(tex1, textureCoord.st).rgb, 1.0);\n"
        "   gl_FragColor = vec4(color.bgr, texture2D(tex1, textureCoord.st).a);\n"
        "}\n";

static const char *qt_glsl_yuvPlanarShaderProgram =
        "uniform sampler2D tex1;\n"
        "uniform sampler2D tex2;\n"
        "uniform sampler2D tex3;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 plane1TexCoord;\n"
        "varying highp vec2 plane2TexCoord;\n"
        "varying highp vec2 plane3TexCoord;\n"
        "void main(void)\n"
        "{\n"
        "   mediump float Y = texture2D(tex1, plane1TexCoord).r;\n"
        "   mediump float U = texture2D(tex2, plane2TexCoord).r;\n"
        "   mediump float V = texture2D(tex3, plane3TexCoord).r;\n"
        "   mediump vec4 color = vec4(Y, U, V, 1.);\n"
        "   gl_FragColor = colorMatrix * color;\n"
        "}\n";

static const char *qt_glsl_nvPlanarShaderProgram =
        "uniform sampler2D tex1;\n"
        "uniform sampler2D tex2;\n"
        "uniform mediump mat4 colorMatrix;\n"
        "varying highp vec2 plane1TexCoord;\n"
        "varying highp vec2 plane2TexCoord;\n"
        "void main()\n"
        "{\n"
        "    mediump float Y = texture2D(tex1, plane1TexCoord).r;\n"
        "    mediump vec2 UV = texture2D(tex2, plane2TexCoord).ra;\n"
        "    mediump vec4 color = vec4(Y, UV.x, UV.y, 1.);\n"
        "    gl_FragColor = colorMatrix * color;\n"
        "}\n";

class QAVWidget_OpenGLPrivate
{
    Q_DECLARE_PUBLIC(QAVWidget_OpenGL)
public:
    QAVWidget_OpenGLPrivate(QAVWidget_OpenGL *q)
        : q_ptr(q)
    {
    }

    QAVWidget_OpenGL *q_ptr = nullptr;

    QAVVideoFrame currentFrame;
    AVPixelFormat currentFormat = AV_PIX_FMT_NONE;
    mutable QMutex mutex;

    QOpenGLShaderProgram program;
    const char *fragmentProgram = nullptr;

    int textureCount = 0;
    bool texturesGenerated = false;
    GLuint textureIds[3];
    GLfloat planeWidth[3];
    QMatrix4x4 colorMatrix;

    void cleanupTextures();
    void bindTexture(int id, int w, int h, const uchar *bits, GLenum format);

    bool resetGL(const QAVVideoFrame &frame);

    template<AVPixelFormat fmt>
    void initTextureInfo(const QAVVideoFrame &frame);

    bool setVideoFrame(const QAVVideoFrame &frame);

    static QMatrix4x4 getColorMatrix(const QAVVideoFrame &frame);
};

void QAVWidget_OpenGLPrivate::cleanupTextures()
{
    Q_Q(QAVWidget_OpenGL);
    if (texturesGenerated)
        q->glDeleteTextures(textureCount, textureIds);
    texturesGenerated = false;
}

QMatrix4x4 QAVWidget_OpenGLPrivate::getColorMatrix(const QAVVideoFrame &frame)
{
    switch (frame.frame()->colorspace) {
        case AVCOL_SPC_BT709:
            return QMatrix4x4(
                    1.164f,  0.000f,  1.793f, -0.5727f,
                    1.164f, -0.534f, -0.213f,  0.3007f,
                    1.164f,  2.115f,  0.000f, -1.1302f,
                    0.0f,    0.000f,  0.000f,  1.0000f);
        case AVCOL_SPC_BT470BG: // BT601
        case AVCOL_SPC_SMPTE170M: // Also BT601
        default:
            return QMatrix4x4(
                    1.164f,  0.000f,  1.596f, -0.8708f,
                    1.164f, -0.392f, -0.813f,  0.5296f,
                    1.164f,  2.017f,  0.000f, -1.081f,
                    0.0f,    0.000f,  0.000f,  1.0000f);
    }
}

QAVWidget_OpenGL::QAVWidget_OpenGL(QWidget *parent)
    : QOpenGLWidget(parent)
    , d_ptr(new QAVWidget_OpenGLPrivate(this))
{
}

QAVWidget_OpenGL::~QAVWidget_OpenGL()
{
    Q_D(QAVWidget_OpenGL);
    QMutexLocker lock(&d->mutex);
    d->cleanupTextures();
}

void QAVWidget_OpenGL::setVideoFrame(const QAVVideoFrame &frame)
{
    Q_D(QAVWidget_OpenGL);
    {
        QMutexLocker lock(&d->mutex);
        d->currentFrame = frame;
    }
    update();
}

void QAVWidget_OpenGLPrivate::bindTexture(int id, int w, int h, const uchar *bits, GLenum format)
{
    Q_Q(QAVWidget_OpenGL);
    q->glBindTexture(GL_TEXTURE_2D, id);
    q->glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, bits);
    q->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    q->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    q->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    q->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_BGR32>(const QAVVideoFrame &frame)
{
    Q_Q(QAVWidget_OpenGL);
    fragmentProgram = qt_glsl_rgbShaderProgram;
    textureCount = 1;
    if (frame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = frame.size().width();
        auto h = frame.size().height();
        auto data = frame.map();
        q->glActiveTexture(GL_TEXTURE0);
        bindTexture(textureIds[0], w, h, data.data[0], GL_RGBA);
    }
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_RGB32>(const QAVVideoFrame &frame)
{
    Q_Q(QAVWidget_OpenGL);
    fragmentProgram = qt_glsl_bgrShaderProgram;
    textureCount = 1;
    if (frame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = frame.size().width();
        auto h = frame.size().height();
        auto data = frame.map();
        q->glActiveTexture(GL_TEXTURE0);
        bindTexture(textureIds[0], w, h, data.data[0], GL_RGBA);
    }
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_VDPAU>(const QAVVideoFrame &frame)
{
    Q_ASSERT(frame.handleType() == QAVVideoFrame::GLTextureHandle);
    initTextureInfo<AV_PIX_FMT_BGR32>(frame);
    cleanupTextures();
    textureIds[0] = frame.handle().toInt();
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_YUV420P>(const QAVVideoFrame &frame)
{
    Q_Q(QAVWidget_OpenGL);
    colorMatrix = getColorMatrix(frame);
    fragmentProgram = qt_glsl_yuvPlanarShaderProgram;
    textureCount = 3;
    if (frame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = frame.size().width();
        auto h = frame.size().height();
        auto data = frame.map();
        planeWidth[0] = qreal(w) / data.bytesPerLine[0];
        planeWidth[1] = planeWidth[2] = qreal(w) / (2 * data.bytesPerLine[1]);
        q->glActiveTexture(GL_TEXTURE1);
        bindTexture(textureIds[1], data.bytesPerLine[1], h / 2, data.data[1], GL_LUMINANCE);
        q->glActiveTexture(GL_TEXTURE2);
        bindTexture(textureIds[2], data.bytesPerLine[2], h / 2, data.data[2], GL_LUMINANCE);
        q->glActiveTexture(GL_TEXTURE0);
        bindTexture(textureIds[0], data.bytesPerLine[0], h, data.data[0], GL_LUMINANCE);
    }
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_NV12>(const QAVVideoFrame &frame)
{
    Q_Q(QAVWidget_OpenGL);
    colorMatrix = getColorMatrix(frame);
    fragmentProgram = qt_glsl_nvPlanarShaderProgram;
    textureCount = 2;
    if (frame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = frame.size().width();
        auto h = frame.size().height();
        auto data = frame.map();
        planeWidth[0] = planeWidth[1] = qreal(w) / data.bytesPerLine[0];
        planeWidth[2] = 0;
        q->glActiveTexture(GL_TEXTURE1);
        bindTexture(textureIds[1], data.bytesPerLine[1] / 2, h / 2, data.data[1], GL_LUMINANCE_ALPHA);
        q->glActiveTexture(GL_TEXTURE0);
        bindTexture(textureIds[0], data.bytesPerLine[0], h, data.data[0], GL_LUMINANCE);
    }
}

bool QAVWidget_OpenGLPrivate::setVideoFrame(const QAVVideoFrame &frame)
{
    if (!frame)
        return false;
    switch (frame.format()) {
        case AV_PIX_FMT_BGR32:
            initTextureInfo<AV_PIX_FMT_BGR32>(frame);
            break;
        case AV_PIX_FMT_RGB32:
            initTextureInfo<AV_PIX_FMT_RGB32>(frame);
            break;
        case AV_PIX_FMT_VDPAU:
            initTextureInfo<AV_PIX_FMT_VDPAU>(frame);
            break;
        case AV_PIX_FMT_YUV420P:
            initTextureInfo<AV_PIX_FMT_YUV420P>(frame);
            break;
        case AV_PIX_FMT_NV12:
            initTextureInfo<AV_PIX_FMT_NV12>(frame);
            break;
        default:
            qWarning() << "Pixel format is not supported:" << frame.formatName() << "AVPixelFormat(" << frame.format() << ")";
            return false;
    }
    return true;
}


bool QAVWidget_OpenGLPrivate::resetGL(const QAVVideoFrame &frame)
{
    // No need to reset
    if (currentFormat == frame.format())
        return true;

    if (!fragmentProgram) {
        qWarning() << "fragmentProgram not found";
        return false;
    }
    program.removeAllShaders();
    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, qt_glsl_vertexShaderProgram)) {
        qWarning() << "Vertex compile error:" << qPrintable(program.log());
        return false;
    }
    if (!program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentProgram)) {
        qWarning() << "Shader compile error:" << qPrintable(program.log());
        program.removeAllShaders();
        return false;
    }
    if (!program.link()) {
        qWarning() << "Shader link error:" << qPrintable(program.log());
        program.removeAllShaders();
        return false;
    }

    currentFormat = frame.format();
    return true;
}

void QAVWidget_OpenGL::initializeGL()
{
    initializeOpenGLFunctions();
    glColor3f(0.0, 0.0, 1.0);
}

void QAVWidget_OpenGL::paintGL()
{
    Q_D(QAVWidget_OpenGL);
    QMutexLocker lock(&d->mutex);
    if (!d->setVideoFrame(d->currentFrame) || !d->resetGL(d->currentFrame)) {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(0.0, 0.0, 1.0);

    bool stencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
    bool scissorTestEnabled = glIsEnabled(GL_SCISSOR_TEST);
    if (stencilTestEnabled)
        glEnable(GL_STENCIL_TEST);
    if (scissorTestEnabled)
        glEnable(GL_SCISSOR_TEST);

    auto frameSize = d->currentFrame.size();
    int width = this->width();
    int height = this->height();

    QRectF rect(0.0, 0.0, width, height);
    QSizeF size = frameSize;
    size.scale(rect.size(), Qt::KeepAspectRatio);
    QRectF target(0, 0, size.width(), size.height());
    target.moveCenter(rect.center());
    QRectF source(0, 0, frameSize.width(), frameSize.height());
    QRectF viewport(0, 0, width, height);

    const GLfloat wfactor = 2.0 / width;
    const GLfloat hfactor = -2.0 / height;

    const QTransform transform;
    const GLfloat positionMatrix[4][4] =
    {
        {
            /*(0,0)*/ GLfloat(wfactor * transform.m11() - transform.m13()),
            /*(0,1)*/ GLfloat(hfactor * transform.m12() + transform.m13()),
            /*(0,2)*/ 0.0,
            /*(0,3)*/ GLfloat(transform.m13())
        }, {
            /*(1,0)*/ GLfloat(wfactor * transform.m21() - transform.m23()),
            /*(1,1)*/ GLfloat(hfactor * transform.m22() + transform.m23()),
            /*(1,2)*/ 0.0,
            /*(1,3)*/ GLfloat(transform.m23())
        }, {
            /*(2,0)*/ 0.0,
            /*(2,1)*/ 0.0,
            /*(2,2)*/ -1.0,
            /*(2,3)*/ 0.0
        }, {
            /*(3,0)*/ GLfloat(wfactor * transform.dx() - transform.m33()),
            /*(3,1)*/ GLfloat(hfactor * transform.dy() + transform.m33()),
            /*(3,2)*/ 0.0,
            /*(3,3)*/ GLfloat(transform.m33())
        }
    };

    const QRectF normalizedViewport(viewport.x() / frameSize.width(),
                                    viewport.y() / frameSize.height(),
                                    viewport.width() / frameSize.width(),
                                    viewport.height() / frameSize.height());

    const GLfloat vertexCoordArray[] =
    {
        GLfloat(target.left())     , GLfloat(target.bottom() + 1),
        GLfloat(target.right() + 1), GLfloat(target.bottom() + 1),
        GLfloat(target.left())     , GLfloat(target.top()),
        GLfloat(target.right() + 1), GLfloat(target.top())
    };

    const bool mirrored = false;
    const bool scanLineDirection = true;
    const GLfloat txLeft = mirrored ? source.right() / frameSize.width()
                                    : source.left() / frameSize.width();
    const GLfloat txRight = mirrored ? source.left() / frameSize.width()
                                     : source.right() / frameSize.width();
    const GLfloat txTop = scanLineDirection
            ? source.top() / frameSize.height()
            : source.bottom() / frameSize.height();
    const GLfloat txBottom = scanLineDirection
            ? source.bottom() / frameSize.height()
            : source.top() / frameSize.height();

    const GLfloat textureCoordArray[] =
    {
        txLeft , txBottom,
        txRight, txBottom,
        txLeft , txTop,
        txRight, txTop
    };

    d->program.bind();

    d->program.enableAttributeArray("vertexCoordArray");
    d->program.enableAttributeArray("textureCoordArray");
    d->program.setAttributeArray("vertexCoordArray", vertexCoordArray, 2);
    d->program.setAttributeArray("textureCoordArray", textureCoordArray, 2);
    d->program.setUniformValue("positionMatrix", positionMatrix);

    for (int i = 0; i < d->textureCount; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, d->textureIds[i]);
    }
    d->program.setUniformValue("colorMatrix", d->colorMatrix);
    d->program.setUniformValue("tex1", 0);
    d->program.setUniformValue("planeWidth1", d->planeWidth[0]);
    if (d->textureCount > 1) {
        d->program.setUniformValue("tex2", 1);
        d->program.setUniformValue("planeWidth2", d->planeWidth[1]);
    }
    if (d->textureCount > 2) {
        d->program.setUniformValue("tex3", 2);
        d->program.setUniformValue("planeWidth3", d->planeWidth[2]);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    d->program.release();
    d->currentFrame = QAVVideoFrame();
}

void QAVWidget_OpenGL::resizeGL(int width, int height)
{
    int side = qMin(width, height);
    glViewport((width - side) / 2, (height - side) / 2, side, side);
}
