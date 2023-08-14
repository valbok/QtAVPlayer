/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFILTERS_P_H
#define QAVFILTERS_P_H

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
#include "qavframe.h"
#include "qavfilter_p.h"
#include "qavdemuxer_p.h"
#include "qavfiltergraph_p.h"
#include <QMutex>
#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVFilters
{
public:
    QAVFilters() = default;
    int createFilters(
        const QList<QString> &filterDescs,
        const QAVFrame &frame,
        const QAVDemuxer &demuxer);
    int write(
        AVMediaType mediaType,
        const QAVFrame &decodedFrame);
    int read(
        AVMediaType mediaType,
        const QAVFrame &decodedFrame,
        QList<QAVFrame> &filteredFrames);
    QList<QString> filterDescs() const;
    bool isEmpty() const;
    void flush();
    void clear();

private:
    Q_DISABLE_COPY(QAVFilters)

    QList<QString> m_filterDescs;
    std::vector<std::unique_ptr<QAVFilterGraph>> m_filterGraphs;
    std::vector<std::unique_ptr<QAVFilter>> m_videoFilters;
    std::vector<std::unique_ptr<QAVFilter>> m_audioFilters;
    mutable QMutex m_mutex;
};

QT_END_NAMESPACE

#endif
