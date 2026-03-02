/*********************************************************
 * Copyright (C) 2025, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavwidget_opengl.h"
#include "qavvideobuffer_p.h"
#include <QOpenGLShaderProgram>
#include <QMutexLocker>
#include <qmath.h>
#include <QDebug>

#if defined(Q_OS_WIN)
#include "qavhwdevice_d3d11_p.h"
#endif

static const char *vertexShaderProgram = R"(
    attribute highp vec4 vertexCoordArray;
    attribute highp vec2 textureCoordArray;
    uniform highp mat4 positionMatrix;
    uniform highp float planeWidth1;
    uniform highp float planeWidth2;
    uniform highp float planeWidth3;
    varying highp vec2 plane1TexCoord;
    varying highp vec2 plane2TexCoord;
    varying highp vec2 plane3TexCoord;
    varying highp vec2 textureCoord;
    void main(void)
    {
        plane1TexCoord = textureCoordArray * vec2(planeWidth1, 1);
        plane2TexCoord = textureCoordArray * vec2(planeWidth2, 1);
        plane3TexCoord = textureCoordArray * vec2(planeWidth3, 1);
        gl_Position = positionMatrix * vertexCoordArray;
        textureCoord = textureCoordArray;
    }
)";

static const char *rgbShaderProgram = R"(
    uniform sampler2D tex1;
    varying highp vec2 textureCoord;
    void main(void)
    {
        highp vec4 color = vec4(texture2D(tex1, textureCoord.st).rgb, 1.0);
        gl_FragColor = vec4(color.rgb, texture2D(tex1, textureCoord.st).a);
    }
)";

static const char *bgrShaderProgram = R"(
    uniform sampler2D tex1;
    varying highp vec2 textureCoord;
    void main(void)
    {
        highp vec4 color = vec4(texture2D(tex1, textureCoord.st).rgb, 1.0);
        gl_FragColor = vec4(color.bgr, texture2D(tex1, textureCoord.st).a);
    }
)";

static const char *yuvPlanarShaderProgram = R"(
    uniform sampler2D tex1;
    uniform sampler2D tex2;
    uniform sampler2D tex3;
    uniform mediump mat4 colorMatrix;
    varying highp vec2 plane1TexCoord;
    varying highp vec2 plane2TexCoord;
    varying highp vec2 plane3TexCoord;
    void main(void)
    {
        mediump float Y = texture2D(tex1, plane1TexCoord).r;
        mediump float U = texture2D(tex2, plane2TexCoord).r;
        mediump float V = texture2D(tex3, plane3TexCoord).r;
        mediump vec4 color = vec4(Y, U, V, 1.);
        gl_FragColor = colorMatrix * color;
    }
)";

static const char *nvPlanarShaderProgram = R"(
    uniform sampler2D tex1;
    uniform sampler2D tex2;
    uniform mediump mat4 colorMatrix;
    varying highp vec2 plane1TexCoord;
    varying highp vec2 plane2TexCoord;
    void main()
    {
        mediump float Y = texture2D(tex1, plane1TexCoord).r;
        mediump vec2 UV = texture2D(tex2, plane2TexCoord).ra;
        mediump vec4 color = vec4(Y, UV.x, UV.y, 1.);
        gl_FragColor = colorMatrix * color;
    }
)";

class QAVWidget_OpenGLPrivate
{
    Q_DECLARE_PUBLIC(QAVWidget_OpenGL)
public:
    QAVWidget_OpenGLPrivate(QAVWidget_OpenGL *q) :
        q_ptr(q),
        m_aspectRatioMode(Qt::KeepAspectRatio)
    {
    }

    QAVWidget_OpenGL *q_ptr = nullptr;

    QAVVideoFrame currentFrame;
    AVPixelFormat currentFormat = AV_PIX_FMT_NONE;
    // Custom video buffer
    std::unique_ptr<QAVVideoBuffer> videoBuffer;
    mutable QMutex mutex;

    QOpenGLShaderProgram program;
    const char *fragmentProgram = nullptr;

    int textureCount = 0;
    bool texturesGenerated = false;
    GLuint textureIds[3];
    GLfloat planeWidth[3];
    QMatrix4x4 colorMatrix;

    // Aspect ratio mode for video scaling
    Qt::AspectRatioMode m_aspectRatioMode;

    void cleanupTextures();
    void bindTexture(int id, int w, int h, const uchar *bits, GLenum format);

    bool resetGL();

    template<AVPixelFormat fmt>
    void initTextureInfo();

    bool initTextureInfo();

    static QMatrix4x4 getColorMatrix(const QAVVideoFrame &frame);
};

void QAVWidget_OpenGLPrivate::cleanupTextures()
{
    Q_Q(QAVWidget_OpenGL);
    if (texturesGenerated)
        q->glDeleteTextures(textureCount, textureIds);
    texturesGenerated = false;
    textureIds[0] = textureIds[1] = textureIds[2] = 0;
}

QMatrix4x4 QAVWidget_OpenGLPrivate::getColorMatrix(const QAVVideoFrame &frame)
{
    switch (frame.frame()->colorspace) {
        default:
        case AVCOL_SPC_BT709:
            if (frame.frame()->color_range != AVCOL_RANGE_UNSPECIFIED)
                return {
                    1.0f,  0.0f,       1.5748f,   -0.790488f,
                    1.0f, -0.187324f, -0.468124f,  0.329010f,
                    1.0f,  1.855600f,  0.0f,      -0.931439f,
                    0.0f,  0.0f,       0.0f,       1.0f
                };
            return {
                1.1644f,  0.0000f,  1.7927f, -0.9729f,
                1.1644f, -0.2132f, -0.5329f,  0.3015f,
                1.1644f,  2.1124f,  0.0000f, -1.1334f,
                0.0000f,  0.0000f,  0.0000f,  1.0000f
            };
        case AVCOL_SPC_BT470BG: // BT601
        case AVCOL_SPC_SMPTE170M: // Also BT601
            if (frame.frame()->color_range != AVCOL_RANGE_UNSPECIFIED)
                return {
                    1.f,  0.000f,   1.772f,   -0.886f,
                    1.f, -0.1646f, -0.57135f,  0.36795f,
                    1.f,  1.42f,    0.000f,   -0.71f,
                    0.0f, 0.000f,   0.000f,    1.0000f
                };
            return {
                1.164f,  0.000f,  1.596f, -0.8708f,
                1.164f, -0.392f, -0.813f,  0.5296f,
                1.164f,  2.017f,  0.000f, -1.0810f,
                0.000f,  0.000f,  0.000f,  1.0000f
            };
        case AVCOL_SPC_BT2020_NCL:
        case AVCOL_SPC_BT2020_CL:
            if (frame.frame()->color_range != AVCOL_RANGE_UNSPECIFIED)
                return {
                    1.f,  0.0000f,  1.4746f, -0.7402f,
                    1.f, -0.1646f, -0.5714f,  0.3694f,
                    1.f,  1.8814f,  0.000f,  -0.9445f,
                    0.0f, 0.0000f,  0.000f,   1.0000f
                };
            return {
                1.1644f,  0.000f,   1.6787f, -0.9157f,
                1.1644f, -0.1874f, -0.6504f,  0.3475f,
                1.1644f,  2.1418f,  0.0000f, -1.1483f,
                0.0000f,  0.0000f,  0.0000f,  1.0000f
            };
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

void QAVWidget_OpenGL::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    Q_D(QAVWidget_OpenGL);
    d->m_aspectRatioMode = mode;
    update();
}

Qt::AspectRatioMode QAVWidget_OpenGL::aspectRatioMode() const
{
    Q_D(const QAVWidget_OpenGL);
    return d->m_aspectRatioMode;
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
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_BGR32>()
{
    Q_Q(QAVWidget_OpenGL);
    fragmentProgram = rgbShaderProgram;
    textureCount = 1;
    if (currentFrame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = currentFrame.size().width();
        auto h = currentFrame.size().height();
        auto data = currentFrame.map();
        q->glActiveTexture(GL_TEXTURE0);
        bindTexture(textureIds[0], w, h, data.data[0], GL_RGBA);
    }
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_RGB32>()
{
    Q_Q(QAVWidget_OpenGL);
    fragmentProgram = bgrShaderProgram;
    textureCount = 1;
    if (currentFrame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = currentFrame.size().width();
        auto h = currentFrame.size().height();
        auto data = currentFrame.map();
        q->glActiveTexture(GL_TEXTURE0);
        bindTexture(textureIds[0], w, h, data.data[0], GL_RGBA);
    }
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_VDPAU>()
{
    Q_ASSERT(currentFrame.handleType() == QAVVideoFrame::GLTextureHandle);
    initTextureInfo<AV_PIX_FMT_BGR32>();
    cleanupTextures();
    textureIds[0] = currentFrame.handle().toInt();
}

template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_YUV420P>()
{
    Q_Q(QAVWidget_OpenGL);
    colorMatrix = getColorMatrix(currentFrame);
    fragmentProgram = yuvPlanarShaderProgram;
    textureCount = 3;
    if (currentFrame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = currentFrame.size().width();
        auto h = currentFrame.size().height();
        auto data = currentFrame.map();
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
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_NV12>()
{
    Q_Q(QAVWidget_OpenGL);
    colorMatrix = getColorMatrix(currentFrame);
    fragmentProgram = nvPlanarShaderProgram;
    textureCount = 2;
    if (currentFrame.handleType() == QAVVideoFrame::NoHandle) {
        cleanupTextures();
        q->glGenTextures(textureCount, textureIds);
        texturesGenerated = true;
        auto w = currentFrame.size().width();
        auto h = currentFrame.size().height();
        auto data = currentFrame.map();
        planeWidth[0] = planeWidth[1] = qreal(w) / data.bytesPerLine[0];
        planeWidth[2] = 0;
        q->glActiveTexture(GL_TEXTURE1);
        bindTexture(textureIds[1], data.bytesPerLine[1] / 2, h / 2, data.data[1], GL_LUMINANCE_ALPHA);
        q->glActiveTexture(GL_TEXTURE0);
        bindTexture(textureIds[0], data.bytesPerLine[0], h, data.data[0], GL_LUMINANCE);
    }
}

#if defined(Q_OS_WIN)
template<>
void QAVWidget_OpenGLPrivate::initTextureInfo<AV_PIX_FMT_D3D11>()
{
    Q_ASSERT(currentFrame.handleType() == QAVVideoFrame::D3D11Texture2DHandle);
    initTextureInfo<AV_PIX_FMT_NV12>();

    QAVHWDevice_D3D11_GL device;
    videoBuffer.reset(device.videoBuffer(currentFrame));

    // Convert to opengl textures
    auto handle = videoBuffer->handle();
    Q_ASSERT(handle.canConvert<QList<QVariant>>());
    auto textures = handle.toList();
    Q_ASSERT(textures.size() == 2);
    cleanupTextures();
    textureIds[0] = textures[0].toULongLong();
    textureIds[1] = textures[1].toULongLong();
    planeWidth[0] = planeWidth[1] = 1;
}
#endif

bool QAVWidget_OpenGLPrivate::initTextureInfo()
{
    switch (currentFrame.format()) {
        case AV_PIX_FMT_BGR32:
            initTextureInfo<AV_PIX_FMT_BGR32>();
            break;
        case AV_PIX_FMT_RGB32:
            initTextureInfo<AV_PIX_FMT_RGB32>();
            break;
        case AV_PIX_FMT_VDPAU:
            initTextureInfo<AV_PIX_FMT_VDPAU>();
            break;
        case AV_PIX_FMT_YUV420P:
            initTextureInfo<AV_PIX_FMT_YUV420P>();
            break;
        case AV_PIX_FMT_NV12:
            initTextureInfo<AV_PIX_FMT_NV12>();
            break;
#if defined(Q_OS_WIN)
        case AV_PIX_FMT_D3D11:
            initTextureInfo<AV_PIX_FMT_D3D11>();
            break;
#endif
        default:
            qWarning() << "Pixel format is not supported:" << currentFrame.formatName() << "AVPixelFormat(" << currentFrame.format() << ")";
            return false;
    }
    return true;
}

bool QAVWidget_OpenGLPrivate::resetGL()
{
    // No need to reset
    if (currentFormat == currentFrame.format())
        return true;

    Q_ASSERT(fragmentProgram);
    program.removeAllShaders();
    if (!program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderProgram)) {
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

    currentFormat = currentFrame.format();
    return true;
}

#ifdef DEBUG
void MessageCallback(GLenum, GLenum type,
    GLuint, GLenum severity, GLsizei, const GLchar *message, const void *)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "", type, severity, message);
}
#endif

void QAVWidget_OpenGL::initializeGL()
{
    initializeOpenGLFunctions();
#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
#endif
    glColor3f(0.0, 0.0, 1.0);
}

void QAVWidget_OpenGL::paintGL()
{
    Q_D(QAVWidget_OpenGL);
    QMutexLocker lock(&d->mutex);
    if (!d->currentFrame || !d->initTextureInfo() || !d->resetGL()) {
        return;
    }

    // Calculate the target rectangle for video (considering aspect ratio)
    auto frameSize = d->currentFrame.size();
    int width = this->width();
    int height = this->height();

    QRectF rect(0.0, 0.0, width, height);
    QSizeF size = frameSize;
    size.scale(rect.size(), d->m_aspectRatioMode);
    QRectF target(0, 0, size.width(), size.height());
    target.moveCenter(rect.center());

    // Draw black letterboxing bars first (for areas outside the video)
    // This ensures proper black bars when aspect ratio doesn't match
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Restore the color used by the video renderer
    glColor3f(0.0, 0.0, 1.0);

    bool stencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
    bool scissorTestEnabled = glIsEnabled(GL_SCISSOR_TEST);
    if (stencilTestEnabled)
        glEnable(GL_STENCIL_TEST);
    if (scissorTestEnabled)
        glEnable(GL_SCISSOR_TEST);

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
    d->videoBuffer.reset();
}

void QAVWidget_OpenGL::resizeGL(int width, int height)
{
    // Use the entire widget for rendering, not just a square
    glViewport(0, 0, width, height);
}
