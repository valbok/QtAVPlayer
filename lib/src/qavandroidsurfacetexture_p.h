/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVANDROIDSURFACETEXTURE_H
#define QAVANDROIDSURFACETEXTURE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qobject.h>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtCore/private/qjni_p.h>
#include <QtCore/private/qjnihelpers_p.h>
using JniObject = QJNIObjectPrivate;
using JniEnvironment = QJNIEnvironmentPrivate;
#else
#include <QtCore/qjniobject.h>
using JniObject = QJniObject;
using JniEnvironment = QJniEnvironment;
#endif

#include <QMatrix4x4>

QT_BEGIN_NAMESPACE

class QAVAndroidSurfaceTexture
{
public:
    explicit QAVAndroidSurfaceTexture(quint32 texName = 0);
    ~QAVAndroidSurfaceTexture();

    jobject surfaceTexture();
    jobject surface();
    inline bool isValid() const { return m_surfaceTexture.isValid(); }

    void release(); // API level 14
    void updateTexImage();

    void attachToGLContext(quint32 texName); // API level 16
    void detachFromGLContext(); // API level 16

private:
    JniObject m_surfaceTexture;
    JniObject m_surface;
};

QT_END_NAMESPACE

#endif // QAVANDROIDSURFACETEXTURE_H
