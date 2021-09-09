/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFILTER_P_H
#define QAVFILTER_P_H

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

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QtAVPlayer/qavframe.h>
#include <QObject>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE

class QAVFilterPrivate;
class Q_AVPLAYER_EXPORT QAVFilter : public QObject
{
public:
    ~QAVFilter();

    QString desc() const;

    int write(const QAVFrame &frame);
    QAVFrame read();

    bool eof() const;

protected:
    QAVFilter(QAVFilterPrivate &d, const QString &desc, QObject *parent = nullptr);
    QScopedPointer<QAVFilterPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVFilter)
private:
    Q_DISABLE_COPY(QAVFilter)
};

QT_END_NAMESPACE

#endif
