/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFRAME_H
#define QAVFRAME_H

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QtAVPlayer/qavstreamframe.h>

QT_BEGIN_NAMESPACE

struct AVFrame;
struct AVRational;
class QAVFramePrivate;
class QAVFrame : public QAVStreamFrame
{
public:
    QAVFrame();
    ~QAVFrame();
    QAVFrame(const QAVFrame &other);
    QAVFrame &operator=(const QAVFrame &other);
    operator bool() const;
    AVFrame *frame() const;

    void setFrameRate(const AVRational &value);
    void setTimeBase(const AVRational &value);
    QString filterName() const;
    void setFilterName(const QString &name);

protected:
    QAVFrame(QAVFramePrivate &d);
    Q_DECLARE_PRIVATE(QAVFrame)
};

QT_END_NAMESPACE

#endif
