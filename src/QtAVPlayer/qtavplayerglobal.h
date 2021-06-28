/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QTAVPLAYERGLOBAL_H
#define QTAVPLAYERGLOBAL_H

#include <QtGui/qtguiglobal.h>

QT_BEGIN_NAMESPACE

#ifndef QT_STATIC
#    if defined(QT_BUILD_QTAVPLAYER_LIB)
#        define Q_AVPLAYER_EXPORT Q_DECL_EXPORT
#    else
#        define Q_AVPLAYER_EXPORT Q_DECL_IMPORT
#    endif
#else
#    define Q_AVPLAYER_EXPORT
#endif

QT_END_NAMESPACE
#endif
