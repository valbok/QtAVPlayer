/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QAVFRAME_H
#define QAVFRAME_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <QtAVPlayer/private/qtavplayerglobal_p.h>
#include <QVideoFrame>

QT_BEGIN_NAMESPACE

struct AVFrame;
class QAVFramePrivate;
class QAVCodec;
class Q_AVPLAYER_EXPORT QAVFrame : public QObject
{
public:
    QAVFrame(QObject *parent = nullptr);
    QAVFrame(const QAVCodec *c, QObject *parent = nullptr);
    ~QAVFrame();
    QAVFrame(const QAVFrame &other);
    QAVFrame &operator=(const QAVFrame &other);
    operator bool() const;
    AVFrame *frame() const;
    double pts() const;

protected:
    QScopedPointer<QAVFramePrivate> d_ptr;
    QAVFrame(QAVFramePrivate &d, QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(QAVFrame)
};

QT_END_NAMESPACE

#endif
