/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVAUDIOOUTPUTFILTER_P_H
#define QAVAUDIOOUTPUTFILTER_P_H

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

class QAVAudioOutputFilterPrivate;
class QAVAudioOutputFilter : public QAVInOutFilter
{
public:
    QAVAudioOutputFilter();
    ~QAVAudioOutputFilter();

    int configure(AVFilterGraph *graph, AVFilterInOut *out) override;
};

QT_END_NAMESPACE

#endif
