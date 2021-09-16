/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVVIDEOINPUTFILTER_P_H
#define QAVVIDEOINPUTFILTER_P_H

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

class QAVDemuxer;
class QAVVideoInputFilterPrivate;
class Q_AVPLAYER_EXPORT QAVVideoInputFilter : public QAVInOutFilter
{
public:
    QAVVideoInputFilter(QObject *parent = nullptr);
    QAVVideoInputFilter(const QAVDemuxer *demuxer, QObject *parent = nullptr);
    QAVVideoInputFilter(const QAVVideoInputFilter &other);
    ~QAVVideoInputFilter();
    QAVVideoInputFilter &operator=(const QAVVideoInputFilter &other);
    
    int configure(AVFilterGraph *graph, AVFilterInOut *in) override;

protected:
    Q_DECLARE_PRIVATE(QAVVideoInputFilter)
};

QT_END_NAMESPACE

#endif
