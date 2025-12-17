/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVAUDIOFORMAT_H
#define QAVAUDIOFORMAT_H

#include <QtAVPlayer/qtavplayerglobal.h>

QT_BEGIN_NAMESPACE

class QAVAudioFormat
{
public:
    enum SampleFormat
    {
        Unknown,
        UInt8,
        Int16,
        Int32,
        Float
    };

    SampleFormat sampleFormat() const { return m_sampleFormat; }
    void setSampleFormat(SampleFormat f) { m_sampleFormat = f; }

    int sampleRate() const { return m_sampleRate; }
    void setSampleRate(int sampleRate) { m_sampleRate = sampleRate; }

    int channelCount() const { return m_channelCount; }
    void setChannelCount(int channelCount) { m_channelCount = channelCount; }

    operator bool() const
    {
        return m_sampleFormat != SampleFormat::Unknown && m_sampleRate != 0 && m_channelCount != 0;
    }

    friend bool operator==(const QAVAudioFormat &a, const QAVAudioFormat &b)
    {
        return a.m_sampleRate == b.m_sampleRate &&
               a.m_channelCount == b.m_channelCount &&
               a.m_sampleFormat == b.m_sampleFormat;
    }

    friend bool operator!=(const QAVAudioFormat &a, const QAVAudioFormat &b)
    {
        return !(a == b);
    }

private:
    SampleFormat m_sampleFormat = Unknown;
    int m_sampleRate = 0;
    int m_channelCount = 0;
};

QT_END_NAMESPACE

#endif
