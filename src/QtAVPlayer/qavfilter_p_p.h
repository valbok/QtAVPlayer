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

#include <QString>

QT_BEGIN_NAMESPACE

class QAVFilter;
struct AVFilterGraph;
struct AVFilterContext;
class QAVFilterPrivate
{
    Q_DECLARE_PUBLIC(QAVFilter)
public:
    QAVFilterPrivate(QAVFilter *q) : q_ptr(q) { }
    virtual ~QAVFilterPrivate() = default;

    virtual int reconfigure(const QAVFrame &frame) = 0;

    QAVFilter *q_ptr = nullptr;
    QString desc;
    AVFilterGraph *graph = nullptr;
    AVFilterContext *src = nullptr;
    AVFilterContext *sink = nullptr;
    QAVFrame sourceFrame;
};

QT_END_NAMESPACE

#endif
