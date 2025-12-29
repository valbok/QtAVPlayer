/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qtavplayer/qavaudioframe.h"
#include "qtavplayer/qavaudioconverter.h"

#include "qavframe_p.h"
#include "qavaudiocodec_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

class QAVAudioFramePrivate : public QAVFramePrivate
{
public:
    QAVAudioFormat outAudioFormat;
    QByteArray data;
};

QAVAudioFrame::QAVAudioFrame()
    : QAVFrame(*new QAVAudioFramePrivate)
{
}

QAVAudioFrame::QAVAudioFrame(const QAVFrame &other)
    : QAVFrame(*new QAVAudioFramePrivate)
{
    operator=(other);
}

QAVAudioFrame::QAVAudioFrame(const QAVAudioFrame &other)
    : QAVFrame(*new QAVAudioFramePrivate)
{
    operator=(other);
}

QAVAudioFrame::QAVAudioFrame(const QAVAudioFormat &format, const QByteArray &data)
    : QAVAudioFrame()
{
    Q_D(QAVAudioFrame);
    d->outAudioFormat = format;
    d->data = data;
}

QAVAudioFrame &QAVAudioFrame::operator=(const QAVFrame &other)
{
    Q_D(QAVAudioFrame);
    QAVFrame::operator=(other);
    d->data.clear();

    return *this;
}

QAVAudioFrame &QAVAudioFrame::operator=(const QAVAudioFrame &other)
{
    Q_D(QAVAudioFrame);
    QAVFrame::operator=(other);
    auto rhs = reinterpret_cast<QAVAudioFramePrivate *>(other.d_ptr.get());
    d->outAudioFormat = rhs->outAudioFormat;
    d->data = rhs->data;

    return *this;
}

QAVAudioFrame::operator bool() const
{
    Q_D(const QAVAudioFrame);
    return (d->outAudioFormat &&!d->data.isEmpty()) || QAVFrame::operator bool();
}

static const QAVAudioCodec *audioCodec(const QAVCodec *c)
{
    return reinterpret_cast<const QAVAudioCodec *>(c);
}

QAVAudioFormat QAVAudioFrame::format() const
{
    Q_D(const QAVAudioFrame);
    if (d->outAudioFormat)
        return d->outAudioFormat;

    if (!d->stream)
        return {};

    auto c = audioCodec(d->stream.codec().data());
    if (!c)
        return {};

    auto format = c->audioFormat();
    if (format.sampleFormat() != QAVAudioFormat::Int32)
        format.setSampleFormat(QAVAudioFormat::Int32);

    return format;
}

QByteArray QAVAudioFrame::data() const
{
    auto d = const_cast<QAVAudioFramePrivate *>(reinterpret_cast<QAVAudioFramePrivate *>(d_ptr.get()));
    if (d->data.isEmpty()) {
        d->outAudioFormat = format();
        d->data = QAVAudioConverter().data(*this);
    }
    return d->data;
}

QT_END_NAMESPACE
