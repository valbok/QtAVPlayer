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
    const QAVFrame &frame, const QAVDemuxer &demuxer)
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
                << "video[" << videoInput.size() << "->" << videoOutput.size() << "]"
                << "audio[" << audioInput.size() << "->" << audioOutput.size() << "]";
        }

        m_filterGraphs.push_back(std::move(graph));
    }

    m_filterDescs = filterDescs;
    return 0;
}

static int apply(
    const QAVFrame &decodedFrame,
    const std::vector<std::unique_ptr<QAVFilter>> &filters,
    QList<QAVFrame> &filteredFrames)
{
    int ret = 0;
    QAVFrame frame;
    if (filters.empty()) {
        filteredFrames.append(decodedFrame);
        return ret;
    }

    for (size_t i = 0; i < filters.size() && ret >= 0; ++i) {
        do {
            ret = filters[i]->write(decodedFrame);
        } while (ret == AVERROR(EAGAIN));
        if (ret >= 0) {
            do {
                ret = filters[i]->read(frame);
                if (ret >= 0 && frame)
                    filteredFrames.append(frame);
            } while (ret == AVERROR(EAGAIN) || !filters[i]->eof());
        }
    }
    return ret;
}

int QAVFilters::applyFilters(
    const QAVFrame &decodedFrame,
    QList<QAVFrame> &filteredFrames)
{
    QMutexLocker locker(&m_mutex);
    auto stream = decodedFrame.stream().stream();
    if (stream) {
        switch (stream->codecpar->codec_type) {
        case AVMEDIA_TYPE_VIDEO:
            return apply(decodedFrame, m_videoFilters, filteredFrames);
        case AVMEDIA_TYPE_AUDIO:
            return apply(decodedFrame, m_audioFilters, filteredFrames);
        default:
            qWarning() << QLatin1String("Unsupported codec type:") << stream->codecpar->codec_type;
            break;
        }
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
        if (!filter->eof())
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
