/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVINOUTFILTER_P_P_H
#define QAVINOUTFILTER_P_P_H

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

class QAVInOutFilter;
struct AVFilterContext;
class QAVInOutFilterPrivate
{
public:
    QAVInOutFilterPrivate(QAVInOutFilter *q) : q_ptr(q) { }
    virtual ~QAVInOutFilterPrivate() = default;

    QAVInOutFilter *q_ptr = nullptr;
    AVFilterContext *ctx = nullptr;
    QString name;
};

QT_END_NAMESPACE

#endif
