/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVVIDEOFILTER_P_H
#define QAVVIDEOFILTER_P_H

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

#include "qavfilter_p.h"
#include "qavvideoinputfilter_p.h"
#include "qavvideooutputfilter_p.h"
#include <QMutex>

QT_BEGIN_NAMESPACE

class QAVVideoFilterPrivate;
class QAVVideoFilter : public QAVFilter
{
public:
    QAVVideoFilter(
        const QAVStream &stream,
        const QString &name,
        const QList<QAVVideoInputFilter> &inputs,
        const QList<QAVVideoOutputFilter> &outputs,
        QMutex &mutex);

    int write(const QAVFrame &frame) override;
    int read(QAVFrame &frame) override;
    void flush() override;

protected:
    Q_DECLARE_PRIVATE(QAVVideoFilter)
private:
    Q_DISABLE_COPY(QAVVideoFilter)
};

QT_END_NAMESPACE

#endif
