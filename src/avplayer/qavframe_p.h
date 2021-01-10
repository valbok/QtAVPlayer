/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFRAME_H
#define QAVFRAME_H

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

#include <QObject>
#include "qtavplayerglobal_p.h"
#include <QVideoFrame>

QT_BEGIN_NAMESPACE

struct AVFrame;
class QAVFramePrivate;
class QAVCodec;
class Q_AVPLAYER_EXPORT QAVFrame : public QObject
{
public:
    QAVFrame(QObject *parent = nullptr);
    QAVFrame(const QAVCodec *c, QObject *parent = nullptr);
    ~QAVFrame();
    QAVFrame(const QAVFrame &other);
    QAVFrame &operator=(const QAVFrame &other);
    operator bool() const;
    AVFrame *frame() const;
    double pts() const;

protected:
    QScopedPointer<QAVFramePrivate> d_ptr;
    QAVFrame(QAVFramePrivate &d, QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(QAVFrame)
};

QT_END_NAMESPACE

#endif
