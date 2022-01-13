/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavsubtitleframe.h"
#include "qavstreamframe_p.h"
#include <QDebug>

extern "C" {
#include "libavcodec/avcodec.h"
}

QT_BEGIN_NAMESPACE

class QAVSubtitleFramePrivate : public QAVStreamFramePrivate
{
public:
    QSharedPointer<AVSubtitle> subtitle;
};

static void subtitle_free(AVSubtitle *subtitle)
{
    avsubtitle_free(subtitle);
}

QAVSubtitleFrame::QAVSubtitleFrame(QObject *parent)
    : QAVStreamFrame(*new QAVSubtitleFramePrivate, parent)
{
    Q_D(QAVSubtitleFrame);
    d->subtitle.reset(new AVSubtitle, subtitle_free);
    memset(d->subtitle.data(), 0, sizeof(*d->subtitle.data()));
}

QAVSubtitleFrame::~QAVSubtitleFrame()
{
}

QAVSubtitleFrame::QAVSubtitleFrame(const QAVSubtitleFrame &other)
    : QAVSubtitleFrame(nullptr)
{
    operator=(other);
}

QAVSubtitleFrame &QAVSubtitleFrame::operator=(const QAVSubtitleFrame &other)
{
    Q_D(QAVSubtitleFrame);
    QAVStreamFrame::operator=(other);
    d->subtitle = static_cast<QAVSubtitleFramePrivate *>(other.d_ptr.data())->subtitle;

    return *this;
}

AVSubtitle *QAVSubtitleFrame::subtitle() const
{
    Q_D(const QAVSubtitleFrame);
    return d->subtitle.data();
}

QT_END_NAMESPACE
