/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QTAVPLAYERGLOBAL_H
#define QTAVPLAYERGLOBAL_H

#include <QtCore/qglobal.h>
#include <memory>
#include <QTimer>
#if defined(QT_AVPLAYER_VA_DRM)
#include <QtGui/private/qtgui-config_p.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#    ifdef QT_BUILD_QTAVPLAYER_LIB
#        define Q_AVPLAYER_EXPORT Q_DECL_EXPORT
#    else
#        define Q_AVPLAYER_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define Q_AVPLAYER_EXPORT
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
template <typename T, typename Deleter>
Q_DECL_CONSTEXPR inline T* qGetPtrHelper(const std::unique_ptr<T, Deleter> &p) Q_DECL_NOTHROW {
    return p.get();
}
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
#    define QT_CONFIG(feature) QT_SUPPORTS(feature)
#endif

template <typename Functor>
Q_DECL_CONSTEXPR inline void qtavplayer_invokeMethod(QObject *context, Functor &&function) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QTimer::singleShot(0, context, function);
#else
    QMetaObject::invokeMethod(context, function);
#endif
}

QT_END_NAMESPACE
#endif
