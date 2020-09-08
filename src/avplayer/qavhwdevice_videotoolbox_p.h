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

#ifndef QAVHWDEVICE_VIDEOTOOLBOX_P_H
#define QAVHWDEVICE_VIDEOTOOLBOX_P_H

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

#include "qavhwdevice_p.h"

QT_BEGIN_NAMESPACE

class QAVHWDevice_VideoToolboxPrivate;
class Q_AVPLAYER_EXPORT QAVHWDevice_VideoToolbox : public QObject, public QAVHWDevice
{
public:
    QAVHWDevice_VideoToolbox(QObject *parent = nullptr);
    ~QAVHWDevice_VideoToolbox();

    AVPixelFormat format() const override;
    bool supportsVideoSurface(QAbstractVideoSurface *surface) const override;
    QVideoFrame decode(const QAVVideoFrame &frame) const override;

private:
    QScopedPointer<QAVHWDevice_VideoToolboxPrivate> d_ptr;
    Q_DISABLE_COPY(QAVHWDevice_VideoToolbox)
    Q_DECLARE_PRIVATE(QAVHWDevice_VideoToolbox)
};

QT_END_NAMESPACE

#endif
