/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFILTER_P_P_H
#define QAVFILTER_P_P_H

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

#include <QtAVPlayer/qavframe.h>
#include <QtAVPlayer/qavstream.h>
#include <QList>
#include <QMutex>

QT_BEGIN_NAMESPACE

class QAVFilter;
class QAVFilterPrivate
{
    Q_DECLARE_PUBLIC(QAVFilter)
public:
    QAVFilterPrivate(QAVFilter *q, QMutex &mutex) : q_ptr(q), graphMutex(mutex) { }
    virtual ~QAVFilterPrivate() = default;

    QAVFilter *q_ptr = nullptr;
    QAVStream stream;
    QString name;
    QAVFrame sourceFrame;
    QList<QAVFrame> outputFrames;
    bool isEmpty = true;
    QMutex &graphMutex;
};

QT_END_NAMESPACE

#endif
