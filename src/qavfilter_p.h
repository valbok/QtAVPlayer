/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFILTER_P_H
#define QAVFILTER_P_H

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
#include <QtAVPlayer/qavframe.h>
#include <QtAVPlayer/qavstream.h>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVFilterPrivate;
class QAVFilter
{
public:
    virtual ~QAVFilter();

    virtual int write(const QAVFrame &frame) = 0;
    virtual int read(QAVFrame &frame) = 0;
    // Checks if all frames have been read
    bool isEmpty() const;
    virtual void flush() = 0;

protected:
    QAVFilter(
        const QAVStream &stream,
        const QString &name,
        QAVFilterPrivate &d);
    std::unique_ptr<QAVFilterPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVFilter)
private:
    Q_DISABLE_COPY(QAVFilter)
};

QT_END_NAMESPACE

#endif
