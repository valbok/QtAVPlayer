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

#ifndef QAVPLAYER_H
#define QAVPLAYER_H

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

#include <QtAVPlayer/private/qtavplayerglobal_p.h>
#include <QMediaPlayer>
#include <QUrl>
#include <QScopedPointer>
#include <QObject>

QT_BEGIN_NAMESPACE

class QAVPlayerPrivate;
class QAbstractVideoSurface;
class Q_AVPLAYER_EXPORT QAVPlayer : public QObject
{
    Q_OBJECT
public:
    QAVPlayer(QObject *parent = nullptr);
    ~QAVPlayer();

    void setSource(const QUrl &url);
    QUrl source() const;

    bool isAudioAvailable() const;
    bool isVideoAvailable() const;

    void setVideoSurface(QAbstractVideoSurface *surface);

    QMediaPlayer::State state() const;
    QMediaPlayer::MediaStatus mediaStatus() const;
    qint64 duration() const;
    qint64 position() const;
    int volume() const;
    bool isMuted() const;
    qreal speed() const;

    bool isSeekable() const;
    QMediaPlayer::Error error() const;
    QString errorString() const;

public Q_SLOTS:
    void play();
    void pause();
    void stop();
    void seek(qint64 position);
    void setVolume(int volume);
    void setMuted(bool muted);
    void setSpeed(qreal rate);

Q_SIGNALS:
    void sourceChanged(const QUrl &url);
    void stateChanged(QMediaPlayer::State newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void errorOccurred(QMediaPlayer::Error, const QString &str);
    void durationChanged(qint64 duration);
    void seekableChanged(bool seekable);
    void volumeChanged(int volume);
    void mutedChanged(bool muted);
    void speedChanged(qreal rate);

protected:
    QScopedPointer<QAVPlayerPrivate> d_ptr;

private:
    Q_DISABLE_COPY(QAVPlayer)
    Q_DECLARE_PRIVATE(QAVPlayer)
};

QT_END_NAMESPACE

#endif
