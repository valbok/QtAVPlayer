/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavandroidsurfacetexture_p.h"
#include <QtCore/qmutex.h>
#include <QtCore/qcoreapplication.h>

QT_BEGIN_NAMESPACE

QAVAndroidSurfaceTexture::QAVAndroidSurfaceTexture(quint32 texName)
{
    Q_STATIC_ASSERT(sizeof (jlong) >= sizeof (void *));
    m_surfaceTexture = JniObject("android/graphics/SurfaceTexture", "(I)V", jint(texName));
}

QAVAndroidSurfaceTexture::~QAVAndroidSurfaceTexture()
{
    if (m_surface.isValid())
        m_surface.callMethod<void>("release");

    if (m_surfaceTexture.isValid())
        release();
}

void QAVAndroidSurfaceTexture::release()
{
    m_surfaceTexture.callMethod<void>("release");
}

void QAVAndroidSurfaceTexture::updateTexImage()
{
    if (!m_surfaceTexture.isValid())
        return;

    m_surfaceTexture.callMethod<void>("updateTexImage");
}

jobject QAVAndroidSurfaceTexture::surfaceTexture()
{
    return m_surfaceTexture.object();
}

jobject QAVAndroidSurfaceTexture::surface()
{
    if (!m_surface.isValid()) {
        m_surface = JniObject("android/view/Surface",
                               "(Landroid/graphics/SurfaceTexture;)V",
                               m_surfaceTexture.object());
    }

    return m_surface.object();
}

void QAVAndroidSurfaceTexture::attachToGLContext(quint32 texName)
{
    if (!m_surfaceTexture.isValid())
        return;

    m_surfaceTexture.callMethod<void>("attachToGLContext", "(I)V", texName);
}

void QAVAndroidSurfaceTexture::detachFromGLContext()
{
    if (!m_surfaceTexture.isValid())
        return;

    m_surfaceTexture.callMethod<void>("detachFromGLContext");
}

QT_END_NAMESPACE
