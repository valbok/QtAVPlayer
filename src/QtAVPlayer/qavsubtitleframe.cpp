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
#include "libavformat/avformat.h"
}

QT_BEGIN_NAMESPACE

class QAVSubtitleFramePrivate : public QAVStreamFramePrivate
{
public:
    QSharedPointer<AVSubtitle> subtitle;

    double pts() const override;
    double duration() const override;
};

static void subtitle_free(AVSubtitle *subtitle)
{
    avsubtitle_free(subtitle);
}

QAVSubtitleFrame::QAVSubtitleFrame()
    : QAVStreamFrame(*new QAVSubtitleFramePrivate)
{
    Q_D(QAVSubtitleFrame);
    d->subtitle.reset(new AVSubtitle, subtitle_free);
    memset(d->subtitle.data(), 0, sizeof(*d->subtitle.data()));
}

QAVSubtitleFrame::~QAVSubtitleFrame()
{
}

QAVSubtitleFrame::QAVSubtitleFrame(const QAVSubtitleFrame &other)
    : QAVSubtitleFrame()
{
    operator=(other);
}

QAVSubtitleFrame &QAVSubtitleFrame::operator=(const QAVSubtitleFrame &other)
{
    Q_D(QAVSubtitleFrame);
    QAVStreamFrame::operator=(other);
    d->subtitle = static_cast<QAVSubtitleFramePrivate *>(other.d_ptr.get())->subtitle;

    return *this;
}

AVSubtitle *QAVSubtitleFrame::subtitle() const
{
    Q_D(const QAVSubtitleFrame);
    return d->subtitle.data();
}

double QAVSubtitleFramePrivate::pts() const
{
    if (!subtitle)
        return NAN;
    AVRational tb;
    tb.num = 1;
    tb.den = AV_TIME_BASE;
    return subtitle->pts == AV_NOPTS_VALUE ? NAN : subtitle->pts * av_q2d(tb);
}

double QAVSubtitleFramePrivate::duration() const
{
    if (!subtitle)
        return 0.0;
    return (subtitle->end_display_time - subtitle->start_display_time) / 1000.0;
}

QT_END_NAMESPACE
