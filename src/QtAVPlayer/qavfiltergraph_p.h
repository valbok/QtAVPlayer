/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFILTERGRAPH_P_H
#define QAVFILTERGRAPH_P_H

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

#include "qavvideoinputfilter_p.h"
#include "qavvideooutputfilter_p.h"
#include "qavaudioinputfilter_p.h"
#include "qavaudiooutputfilter_p.h"
#include <QtAVPlayer/qtavplayerglobal.h>
#include <QtAVPlayer/qavframe.h>
#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudioframe.h>
#include <QMutex>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVFilterGraphPrivate;
class QAVDemuxer;
class QAVFilterGraph
{
public:
    QAVFilterGraph();
    ~QAVFilterGraph();

    int parse(const QString &desc);
    int apply(const QAVFrame &frame);
    int config();
    QString desc() const;
    QMutex &mutex();

    AVFilterGraph *graph() const;
    QList<QAVVideoInputFilter> videoInputFilters() const;
    QList<QAVVideoOutputFilter> videoOutputFilters() const;
    QList<QAVAudioInputFilter> audioInputFilters() const;
    QList<QAVAudioOutputFilter> audioOutputFilters() const;

protected:
    std::unique_ptr<QAVFilterGraphPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVFilterGraph)
private:
    Q_DISABLE_COPY(QAVFilterGraph)
};

QT_END_NAMESPACE

#endif
