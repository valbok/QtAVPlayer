/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVINOUTFILTER_P_H
#define QAVINOUTFILTER_P_H

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
#include <memory>

QT_BEGIN_NAMESPACE

struct AVFilterGraph;
struct AVFilterInOut;
struct AVFilterContext;
class QAVInOutFilterPrivate;
class QAVInOutFilter
{
public:
    QAVInOutFilter();
    virtual ~QAVInOutFilter();
    QAVInOutFilter(const QAVInOutFilter &other);
    QAVInOutFilter &operator=(const QAVInOutFilter &other);
    virtual int configure(AVFilterGraph *graph, AVFilterInOut *in);
    AVFilterContext *ctx() const;
    QString name() const;

protected:
    std::unique_ptr<QAVInOutFilterPrivate> d_ptr;
    QAVInOutFilter(QAVInOutFilterPrivate &d);
    Q_DECLARE_PRIVATE(QAVInOutFilter)
};

QT_END_NAMESPACE

#endif
