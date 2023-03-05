/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavfilters_p.h"
#include "qavvideofilter_p.h"
#include "qavaudiofilter_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

int QAVFilters::createFilters(
    const QList<QString> &filterDescs,
    const QAVFrame &frame,
    const QAVDemuxer &demuxer)
{
    QMutexLocker locker(&m_mutex);
    m_videoFilters.clear();
    m_audioFilters.clear();
    m_filterGraphs.clear();
    for (int i = 0; i < filterDescs.size(); ++i) {
        const auto & filterDesc = filterDescs[i];
        std::unique_ptr<QAVFilterGraph> graph(!filterDesc.isEmpty() ? new QAVFilterGraph : nullptr);
        if (graph) {
            int ret = graph->parse(filterDesc);
            if (ret < 0) {
                qWarning() << QLatin1String("Could not parse filter desc:") << filterDesc << ret;
                return ret;
            }
            QAVFrame videoFrame;
            QAVFrame audioFrame;
            videoFrame.setStream(demuxer.videoStream());
            audioFrame.setStream(demuxer.audioStream());
            auto stream = frame.stream().stream();
            if (stream) {
                switch (stream->codecpar->codec_type) {
                case AVMEDIA_TYPE_VIDEO:
                    videoFrame = frame;
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    audioFrame = frame;
                    break;
                default:
                    qWarning() << QLatin1String("Unsupported codec type:") << stream->codecpar->codec_type;
                    return AVERROR(ENOTSUP);
                }
            }
            ret = graph->apply(videoFrame);
            if (ret < 0) {
                qWarning() << QLatin1String("Could not create video filters") << ret;
                return ret;
            }
            ret = graph->apply(audioFrame);
            if (ret < 0) {
                qWarning() << QLatin1String("Could not create audio filters") << ret;
                return ret;
            }
            ret = graph->config();
            if (ret < 0) {
                qWarning() << QLatin1String("Could not configure filter graph") << ret;
                return ret;
            }

            auto videoInput = graph->videoInputFilters();
            auto videoOutput = graph->videoOutputFilters();
            if (!videoInput.isEmpty() && !videoOutput.isEmpty()) {
                m_videoFilters.emplace_back(
                    std::unique_ptr<QAVFilter>(
                        new QAVVideoFilter(QString::number(i), videoInput, videoOutput)
                    )
                );
            }
            auto audioInput = graph->audioInputFilters();
            auto audioOutput = graph->audioOutputFilters();
            if (!audioInput.isEmpty() && !audioOutput.isEmpty()) {
                m_audioFilters.emplace_back(
                    std::unique_ptr<QAVFilter>(
                        new QAVAudioFilter(QString::number(i), audioInput, audioOutput)
                    )
                );
            }
            qDebug() << __FUNCTION__ << ":" << filterDesc
                << "video[input:" << videoInput.size() << "-> output:" << videoOutput.size() << "]"
                << "audio[input:" << audioInput.size() << "-> output:" << audioOutput.size() << "]";
        }

        m_filterGraphs.push_back(std::move(graph));
    }

    m_filterDescs = filterDescs;
    return 0;
}

static int writeFrame(
    const QAVFrame &decodedFrame,
    const std::vector<std::unique_ptr<QAVFilter>> &filters)
{
    int ret = 0;
    for (size_t i = 0; i < filters.size() && ret >= 0; ++i)
        ret = filters[i]->write(decodedFrame);
    return ret;
}

int QAVFilters::write(
    AVMediaType mediaType,
    const QAVFrame &decodedFrame)
{
    QMutexLocker locker(&m_mutex);
    switch (mediaType) {
    case AVMEDIA_TYPE_VIDEO:
        return writeFrame(decodedFrame, m_videoFilters);
    case AVMEDIA_TYPE_AUDIO:
        return writeFrame(decodedFrame, m_audioFilters);
    default:
        qWarning() << QLatin1String("Unsupported codec type:") << mediaType;
        break;
    }
    return AVERROR(ENOTSUP);
}

static int readFrames(
    const QAVFrame &decodedFrame,
    const std::vector<std::unique_ptr<QAVFilter>> &filters,
    QList<QAVFrame> &filteredFrames)
{
    QAVFrame frame;
    if (filters.empty()) {
        filteredFrames.append(decodedFrame);
        return 0;
    }

    // Read all frames from all filters at once
    int ret = 0;
    for (size_t i = 0; i < filters.size() && ret >= 0; ++i) {
        do {
            ret = filters[i]->read(frame);
            if (ret >= 0 && frame)
                filteredFrames.append(frame);
        } while (!filters[i]->isEmpty());
    }

    return ret;
}

int QAVFilters::read(
    AVMediaType mediaType,
    const QAVFrame &decodedFrame,
    QList<QAVFrame> &filteredFrames)
{
    QMutexLocker locker(&m_mutex);
    switch (mediaType) {
    case AVMEDIA_TYPE_VIDEO:
        return readFrames(decodedFrame, m_videoFilters, filteredFrames);
    case AVMEDIA_TYPE_AUDIO:
        return readFrames(decodedFrame, m_audioFilters, filteredFrames);
    default:
        qWarning() << QLatin1String("Unsupported codec type:") << mediaType;
        break;
    }
    return AVERROR(ENOTSUP);
}

QList<QString> QAVFilters::filterDescs() const
{
    QMutexLocker locker(&m_mutex);
    return m_filterDescs;
}

static bool filtersEmpty(const std::vector<std::unique_ptr<QAVFilter>> &filters)
{
    for (auto &filter : filters)
        if (!filter->isEmpty())
            return false;
    return true;
}

bool QAVFilters::isEmpty() const
{
    QMutexLocker locker(&m_mutex);
    return filtersEmpty(m_videoFilters) && filtersEmpty(m_audioFilters);
}

void QAVFilters::clear()
{
    QMutexLocker locker(&m_mutex);
    m_videoFilters.clear();
    m_audioFilters.clear();
    m_filterGraphs.clear();
}

QT_END_NAMESPACE
