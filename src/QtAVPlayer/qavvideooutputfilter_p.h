/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVVIDEOOUTPUTFILTER_P_H
#define QAVVIDEOOUTPUTFILTER_P_H

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

#include "qavinoutfilter_p.h"

QT_BEGIN_NAMESPACE

class QAVVideoOutputFilterPrivate;
class QAVVideoOutputFilter : public QAVInOutFilter
{
public:
    QAVVideoOutputFilter();
    ~QAVVideoOutputFilter();

    int configure(AVFilterGraph *graph, AVFilterInOut *out) override;
};

QT_END_NAMESPACE

#endif
