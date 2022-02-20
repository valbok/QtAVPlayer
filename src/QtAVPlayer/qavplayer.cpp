/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"
#include "qavdemuxer_p.h"
#include "qaviodevice_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavvideoframe.h"
#include "qavaudioframe.h"
#include "qavsubtitleframe.h"
#include "qavpacketqueue_p.h"
#include "qavfiltergraph_p.h"
#include "qavvideofilter_p.h"
#include "qavaudiofilter_p.h"
#include <QtConcurrent>
#include <QLoggingCategory>
#include <functional>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcAVPlayer, "qt.QtAVPlayer")

enum PendingMediaStatus
{
    LoadingMedia,
    PlayingMedia,
    PausingMedia,
    StoppingMedia,
    SteppingMedia,
    SeekingMedia,
    EndOfMedia
};

class QAVPlayerPrivate
{
    Q_DECLARE_PUBLIC(QAVPlayer)
public:
    QAVPlayerPrivate(QAVPlayer *q)
        : q_ptr(q)
        , videoQueue(demuxer)
        , audioQueue(demuxer)
        , subtitleQueue(demuxer)
    {
        threadPool.setMaxThreadCount(4);
    }

    QAVPlayer::Error currentError() const;
    void setMediaStatus(QAVPlayer::MediaStatus status);
    void resetPendingStatuses();
    void setPendingMediaStatus(PendingMediaStatus status);
    void step(bool hasFrame);
    bool doStep(PendingMediaStatus status, bool hasFrame);
    bool setState(QAVPlayer::State s);
    void setSeekable(bool seekable);
    void setError(QAVPlayer::Error err, const QString &str);
    void setDuration(double d);
    bool isSeeking() const;
    bool isEndOfFile() const;
    void endOfFile(bool v);
    void setVideoFrameRate(double v);
    double pts() const;
    void applyFilter(bool reset = false);

    void terminate();

    void doWait();
    void wait(bool v);
    void doLoad();
    void doDemux();
    bool skipFrame(const QAVStreamFrame &frame, const QAVPacketQueue &queue, bool master);
    bool doPlayStep(double refPts, bool master, QAVPacketQueue &queue, bool &sync, QAVStreamFrame &frame);
    void doPlayVideo();
    void doPlayAudio();
    void doPlaySubtitle();

    template <class T>
    void dispatch(T fn);

    QAVPlayer *q_ptr = nullptr;
    QString url;
    QScopedPointer<QAVIODevice> dev;
    QAVPlayer::MediaStatus mediaStatus = QAVPlayer::NoMedia;
    QList<PendingMediaStatus> pendingMediaStatuses;
    QAVPlayer::State state = QAVPlayer::StoppedState;
    mutable QMutex stateMutex;

    bool seekable = false;
    qreal speed = 1.0;
    mutable QMutex speedMutex;
    double videoFrameRate = 0.0;

    double duration = 0;
    double pendingPosition = 0;
    bool pendingSeek = false;
    mutable QMutex positionMutex;
    bool synced = true;

    int videoStream = -1;
    int audioStream = -1;
    int subtitleStream = -1;

    QAVPlayer::Error error = QAVPlayer::NoError;

    QAVDemuxer demuxer;

    QThreadPool threadPool;
    QFuture<void> loaderFuture;
    QFuture<void> demuxerFuture;

    QFuture<void> videoPlayFuture;
    QAVPacketFrameQueue videoQueue;

    QFuture<void> audioPlayFuture;
    QAVPacketFrameQueue audioQueue;

    QFuture<void> subtitlePlayFuture;
    QAVPacketSubtitleQueue subtitleQueue;

    bool quit = 0;
    bool isWaiting = false;
    mutable QMutex waitMutex;
    QWaitCondition waitCond;
    bool eof = false;

    QString filterDesc;
    QScopedPointer<QAVFilterGraph> filterGraph;
};

static QString err_str(int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));

    return QString::fromUtf8(errbuf_ptr);
}

QAVPlayer::Error QAVPlayerPrivate::currentError() const
{
    QMutexLocker locker(&stateMutex);
    return error;
}

void QAVPlayerPrivate::setMediaStatus(QAVPlayer::MediaStatus status)
{
    {
        QMutexLocker locker(&stateMutex);
        if (mediaStatus == status)
            return;

        if (status != QAVPlayer::InvalidMedia)
            error = QAVPlayer::NoError;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << mediaStatus << "->" << status;
        mediaStatus = status;
    }

    emit q_ptr->mediaStatusChanged(status);
}

void QAVPlayerPrivate::resetPendingStatuses()
{
    QMutexLocker locker(&stateMutex);
    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << pendingMediaStatuses;
    pendingMediaStatuses.clear();
    wait(true);
}

void QAVPlayerPrivate::setPendingMediaStatus(PendingMediaStatus status)
{
    QMutexLocker locker(&stateMutex);
    pendingMediaStatuses.push_back(status);
    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << mediaStatus << "->" << pendingMediaStatuses;
}

bool QAVPlayerPrivate::setState(QAVPlayer::State s)
{
    Q_Q(QAVPlayer);
    bool result = false;
    {
        QMutexLocker locker(&stateMutex);
        if (state == s)
            return result;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << state << "->" << s;
        state = s;
        result = true;
    }

    emit q->stateChanged(s);
    return result;
}

void QAVPlayerPrivate::setSeekable(bool s)
{
    Q_Q(QAVPlayer);
    if (seekable == s)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << seekable << "->" << s;
    seekable = s;
    emit q->seekableChanged(seekable);
}

void QAVPlayerPrivate::setDuration(double d)
{
    Q_Q(QAVPlayer);
    if (qFuzzyCompare(duration, d))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << duration << "->" << d;
    duration = d;
    emit q->durationChanged(q->duration());
}

bool QAVPlayerPrivate::isSeeking() const
{
    QMutexLocker locker(&positionMutex);
    return pendingSeek;
}

bool QAVPlayerPrivate::isEndOfFile() const
{
    QMutexLocker locker(&stateMutex);
    return eof;
}

void QAVPlayerPrivate::endOfFile(bool v)
{
    QMutexLocker locker(&stateMutex);
    eof = v;
}

void QAVPlayerPrivate::setVideoFrameRate(double v)
{
    Q_Q(QAVPlayer);
    if (qFuzzyCompare(videoFrameRate, v))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << videoFrameRate << "->" << v;
    videoFrameRate = v;
    emit q->videoFrameRateChanged(v);
}

double QAVPlayerPrivate::pts() const
{
    Q_Q(const QAVPlayer);
    return !q->videoStreams().isEmpty() ? videoQueue.pts() : audioQueue.pts();
}

template <class T>
void QAVPlayerPrivate::dispatch(T fn)
{
    QMetaObject::invokeMethod(q_ptr, fn, nullptr);
}

void QAVPlayerPrivate::setError(QAVPlayer::Error err, const QString &str)
{
    Q_Q(QAVPlayer);
    {
        QMutexLocker locker(&stateMutex);
        error = err;
    }

    qWarning() << err << ":" << str;
    emit q->errorOccurred(err, str);
    setMediaStatus(QAVPlayer::InvalidMedia);
    setState(QAVPlayer::StoppedState);
    resetPendingStatuses();
}

void QAVPlayerPrivate::terminate()
{
    qCDebug(lcAVPlayer) << __FUNCTION__;
    setState(QAVPlayer::StoppedState);
    quit = true;
    wait(false);
    videoFrameRate = 0.0;
    videoQueue.clear();
    videoQueue.abort();
    audioQueue.clear();
    audioQueue.abort();
    subtitleQueue.clear();
    subtitleQueue.abort();
    if (dev)
        dev->abort(true);
    loaderFuture.waitForFinished();
    demuxerFuture.waitForFinished();
    videoPlayFuture.waitForFinished();
    audioPlayFuture.waitForFinished();
    pendingPosition = 0;
    pendingSeek = false;
    pendingMediaStatuses.clear();
    filterGraph.reset();
    setDuration(0);
    error = QAVPlayer::NoError;
    dev.reset();
    eof = false;
}

void QAVPlayerPrivate::step(bool hasFrame)
{
    QMutexLocker locker(&stateMutex);
    while (!pendingMediaStatuses.isEmpty()) {
        auto status = pendingMediaStatuses.first();
        locker.unlock();
        if (!doStep(status, hasFrame))
            break;
        locker.relock();
        if (!pendingMediaStatuses.isEmpty()) {
            pendingMediaStatuses.removeFirst();
            qCDebug(lcAVPlayer) << "Step done:" << status << ", pending" << pendingMediaStatuses;
        }
    }

    if (pendingMediaStatuses.isEmpty()) {
        videoQueue.wake(false);
        audioQueue.wake(false);
        subtitleQueue.wake(false);
    } else {
        wait(false);
    }
}

bool QAVPlayerPrivate::doStep(PendingMediaStatus status, bool hasFrame)
{
    bool result = false;
    const bool valid = hasFrame && !isSeeking() && q_ptr->mediaStatus() != QAVPlayer::NoMedia;
    switch (status) {
        case PlayingMedia:
            if (valid) {
                result = true;
                qCDebug(lcAVPlayer) << "Played from pos:" << q_ptr->position();
                emit q_ptr->played(q_ptr->position());
                wait(false);
            }
            break;

        case PausingMedia:
            if (valid) {
                result = true;
                qCDebug(lcAVPlayer) << "Paused to pos:" << q_ptr->position();
                emit q_ptr->paused(q_ptr->position());
                wait(true);
            }
            break;

        case SeekingMedia:
            if (valid) {
                result = true;
                if (q_ptr->mediaStatus() == QAVPlayer::EndOfMedia)
                    setMediaStatus(QAVPlayer::LoadedMedia);
                qCDebug(lcAVPlayer) << "Seeked to pos:" << q_ptr->position();
                emit q_ptr->seeked(q_ptr->position());
                QAVPlayer::State currState = q_ptr->state();
                if (currState == QAVPlayer::PausedState || currState == QAVPlayer::StoppedState)
                    wait(true);
            }
            break;

        case StoppingMedia:
            if (q_ptr->mediaStatus() != QAVPlayer::NoMedia) {
                result = true;
                qCDebug(lcAVPlayer) << "Stopped to pos:" << q_ptr->position();
                emit q_ptr->stopped(q_ptr->position());
                wait(true);
            }
            break;

        case SteppingMedia:
            result = isEndOfFile();
            if (valid) {
                result = true;
                qCDebug(lcAVPlayer) << "Stepped to pos:" << q_ptr->position();
                emit q_ptr->stepped(q_ptr->position());
                wait(true);
            }
            break;

        case LoadingMedia:
            result = true;
            setMediaStatus(QAVPlayer::LoadedMedia);
            break;

        case EndOfMedia:
            result = true;
            setMediaStatus(QAVPlayer::EndOfMedia);
            break;

        default:
            break;
    }

    // The step is finished but queues are empty => no more frames will be sent.
    // Need to skip current status and move to next to prevent the blocking.
    if (!result && demuxer.eof() && videoQueue.isEmpty() && audioQueue.isEmpty() && !isSeeking()) {
        result = true;
        qCDebug(lcAVPlayer) << __FUNCTION__ << ": EndOfMedia -> skipping:" << status;
    }

    return result;
}

void QAVPlayerPrivate::doWait()
{
    QMutexLocker lock(&waitMutex);
    if (isWaiting)
        waitCond.wait(&waitMutex);
}

void QAVPlayerPrivate::wait(bool v)
{
    {
        QMutexLocker locker(&waitMutex);
        if (isWaiting != v)
            qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << isWaiting << "->" << v;
        isWaiting = v;
    }

    if (!v)
        waitCond.wakeAll();
    videoQueue.wake(true);
    audioQueue.wake(true);
    subtitleQueue.wake(true);
}

void QAVPlayerPrivate::applyFilter(bool reset)
{
    QMutexLocker locker(&stateMutex);
    if ((filterDesc.isEmpty() && !filterGraph) || (!reset && filterGraph && filterGraph->desc() == filterDesc))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << filterDesc;
    QScopedPointer<QAVFilterGraph> graph(!filterDesc.isEmpty() ? new QAVFilterGraph(demuxer) : nullptr);
    int ret = graph ? graph->parse(filterDesc) : 0;
    if (ret < 0) {
        locker.unlock();
        setError(QAVPlayer::FilterError, QLatin1String("Could not parse filter desc: ") + err_str(ret));
        return;
    }

    videoQueue.setFilter(graph ? new QAVVideoFilter(graph->videoInputFilters(), graph->videoOutputFilters()) : nullptr);
    audioQueue.setFilter(graph ? new QAVAudioFilter(graph->audioInputFilters(), graph->audioOutputFilters()) : nullptr);
    filterGraph.reset(graph.take());

    if (error == QAVPlayer::FilterError) {
        locker.unlock();
        setMediaStatus(QAVPlayer::LoadedMedia);
    }
}

void QAVPlayerPrivate::doLoad()
{
    demuxer.abort(false);
    demuxer.unload();
    int ret = demuxer.load(url, dev.data());
    if (ret < 0) {
        setError(QAVPlayer::ResourceError, err_str(ret));
        return;
    }

    if (!demuxer.videoStream() && !demuxer.audioStream()) {
        setError(QAVPlayer::ResourceError, QLatin1String("No codecs found"));
        return;
    }
    
    applyFilter();
    dispatch([this] {
        qCDebug(lcAVPlayer) << "[" << url << "]: Loaded, seekable:" << demuxer.seekable() << ", duration:" << demuxer.duration();
        setSeekable(demuxer.seekable());
        setDuration(demuxer.duration());
        setVideoFrameRate(demuxer.videoFrameRate());
        step(false);
    });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    demuxerFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doDemux);
    if (!q_ptr->videoStreams().isEmpty())
        videoPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayVideo);
    if (!q_ptr->audioStreams().isEmpty())
        audioPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayAudio);
    if (!q_ptr->subtitleStreams().isEmpty())
        audioPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlaySubtitle);
#else
    demuxerFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doDemux, this);
    if (!q_ptr->videoStreams().isEmpty())
        videoPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayVideo, this);
    if (!q_ptr->audioStreams().isEmpty())
        audioPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayAudio, this);
    if (!q_ptr->subtitleStreams().isEmpty())
        audioPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlaySubtitle, this);
#endif
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

void QAVPlayerPrivate::doDemux()
{
    const int maxQueueBytes = 15 * 1024 * 1024;
    QMutex waiterMutex;
    QWaitCondition waiter;

    while (!quit) {
        if (videoQueue.bytes() + audioQueue.bytes() > maxQueueBytes
            || (videoQueue.enough() && audioQueue.enough()))
        {
            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
            continue;
        }

        {
            QMutexLocker locker(&positionMutex);
            if (pendingSeek) {
                if (pendingPosition < 0)
                    pendingPosition += demuxer.duration();
                if (pendingPosition < 0)
                    pendingPosition = 0;
                const double pos = pendingPosition;
                locker.unlock();
                qCDebug(lcAVPlayer) << "Seeking to pos:" << pos * 1000;
                const int ret = demuxer.seek(pos);
                if (ret >= 0) {
                    qCDebug(lcAVPlayer) << "Waiting video thread finished processing packets";
                    videoQueue.waitForEmpty();
                    qCDebug(lcAVPlayer) << "Waiting audio thread finished processing packets";
                    audioQueue.waitForEmpty();
                    qCDebug(lcAVPlayer) << "Waiting subtitle thread finished processing packets";
                    subtitleQueue.waitForEmpty();
                    qCDebug(lcAVPlayer) << "Reset filters";
                    applyFilter(true);
                    qCDebug(lcAVPlayer) << "Start reading packets from" << pos * 1000;
                } else {
                    qWarning() << "Could not seek:" << ret << ":" << err_str(ret);
                }
                locker.relock();
                if (qFuzzyCompare(pendingPosition, pos))
                    pendingSeek = false;
            }
        }

        auto packet = demuxer.read();
        if (packet) {
            endOfFile(false);
            if (packet.packet()->stream_index == demuxer.videoStream().index())
                videoQueue.enqueue(packet);
            else if (packet.packet()->stream_index == demuxer.audioStream().index())
                audioQueue.enqueue(packet);
            else if (packet.packet()->stream_index == demuxer.subtitleStream().index())
                subtitleQueue.enqueue(packet);
        } else {
            if (demuxer.eof() && videoQueue.isEmpty() && audioQueue.isEmpty() && subtitleQueue.isEmpty() && !isEndOfFile()) {
                endOfFile(true);
                qCDebug(lcAVPlayer) << "EndOfMedia";
                setPendingMediaStatus(EndOfMedia);
                q_ptr->stop();
                wait(false);
            }

            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
        }
    }
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

static double streamDuration(const QAVStreamFrame &frame, const QAVDemuxer &demuxer)
{
    double duration = demuxer.duration();
    const double stream_duration = frame.stream().duration();
    if (stream_duration > 0 && stream_duration < duration)
        duration = stream_duration;
    return duration;
}

static bool isLastFrame(const QAVStreamFrame &frame, const QAVDemuxer &demuxer)
{
    bool result = false;
    if (!isnan(frame.duration()) && frame.duration() > 0) {
        const double requestedPos = streamDuration(frame, demuxer);
        const int frameNumber = frame.pts() / frame.duration();
        const int requestedFrameNumber = requestedPos / frame.duration();
        result = frameNumber + 1 >= requestedFrameNumber;
    }
    return result;
}

bool QAVPlayerPrivate::skipFrame(const QAVStreamFrame &frame, const QAVPacketQueue &queue, bool master = true)
{
    QMutexLocker locker(&positionMutex);
    bool result = pendingSeek;
    if (!pendingSeek && pendingPosition > 0) {
        const bool isQueueEOF = demuxer.eof() && queue.isEmpty();
        // Assume that no frames will be sent after this duration
        const double duration = streamDuration(frame, demuxer);
        const double requestedPos = qMin(pendingPosition, duration);
        double pos = frame.pts();
        // Show last frame if seeked to duration
        bool lastFrame = false;
        if (pendingPosition >= duration) {
            pos += frame.duration();
            // Additional check if frame rate has been changed,
            // thus last frame could be far away from duration by pts,
            // but frame number points to the latest frame.
            lastFrame = isLastFrame(frame, demuxer);
        }
        result = pos < requestedPos && !isQueueEOF && !lastFrame;
        if (master) {
            if (result)
                qCDebug(lcAVPlayer) << __FUNCTION__ << pos << "<" << requestedPos;
            else
                pendingPosition = 0;
        }
    }

    return result;
}

bool QAVPlayerPrivate::doPlayStep(double refPts, bool master, QAVPacketQueue &queue, bool &sync, QAVStreamFrame &frame)
{
    doWait();
    bool hasFrame = false;
    const int ret = queue.frame(synced ? sync : synced, q_ptr->speed(), refPts, frame);
    if (ret < 0) {
        if (ret != AVERROR(EAGAIN)) {
            setError(QAVPlayer::FilterError, err_str(ret));
        } else {
            hasFrame = isLastFrame(frame, demuxer); // Always flush events on the latest frame if filters don't have enough though
            frame = {}; // Don't send the latest frame
        }
    } else {
        if (frame) {
            sync = !skipFrame(frame, queue, master);
            hasFrame = sync;
        }
    }
    if (master)
        step(hasFrame);
    return hasFrame;
}

void QAVPlayerPrivate::doPlayVideo()
{
    videoQueue.setFrameRate(demuxer.videoFrameRate());
    bool sync = true;
    QAVVideoFrame frame;
    const bool master = true;

    while (!quit) {
        if (doPlayStep(audioQueue.pts(), master, videoQueue, sync, frame)) {
            if (frame)
                emit q_ptr->videoFrame(frame);
        }
    }

    videoQueue.clear();
    setMediaStatus(QAVPlayer::NoMedia);
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

void QAVPlayerPrivate::doPlayAudio()
{
    const bool master = q_ptr->videoStreams().isEmpty();
    const double ref = -1;
    bool sync = true;
    QAVAudioFrame frame;

    while (!quit) {
        if (doPlayStep(ref, master, audioQueue, sync, frame)) {
            if (frame)
                emit q_ptr->audioFrame(frame);
        }
    }

    audioQueue.clear();
    if (master)
        setMediaStatus(QAVPlayer::NoMedia);
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

void QAVPlayerPrivate::doPlaySubtitle()
{
    const bool master = false;
    const double ref = -1;
    bool sync = true;
    QAVSubtitleFrame frame;

    while (!quit) {
        if (doPlayStep(ref, master, subtitleQueue, sync, frame)) {
            if (frame)
                emit q_ptr->subtitleFrame(frame);
        }
    }

    subtitleQueue.clear();
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

QAVPlayer::QAVPlayer(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVPlayerPrivate(this))
{
    qRegisterMetaType<QAVAudioFrame>();
    qRegisterMetaType<QAVVideoFrame>();
    qRegisterMetaType<QAVSubtitleFrame>();
    qRegisterMetaType<State>();
    qRegisterMetaType<MediaStatus>();
    qRegisterMetaType<Error>();
    qRegisterMetaType<QAVStream>();
}

QAVPlayer::~QAVPlayer()
{
    Q_D(QAVPlayer);
    d->terminate();
}

void QAVPlayer::setSource(const QString &url, QIODevice *dev)
{
    Q_D(QAVPlayer);
    if (d->url == url)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << url;

    d->terminate();
    d->url = url;
    if (dev)
        d->dev.reset(new QAVIODevice(*dev));
    emit sourceChanged(url);
    d->wait(true);
    d->quit = false;
    if (url.isEmpty())
        return;

    d->setPendingMediaStatus(LoadingMedia);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d->loaderFuture = QtConcurrent::run(&d->threadPool, d, &QAVPlayerPrivate::doLoad);
#else
    d->loaderFuture = QtConcurrent::run(&d->threadPool, &QAVPlayerPrivate::doLoad, d);
#endif
}

QString QAVPlayer::source() const
{
    return d_func()->url;
}

QList<QAVStream> QAVPlayer::videoStreams() const
{
    Q_D(const QAVPlayer);
    return d->demuxer.videoStreams();
}

QAVStream QAVPlayer::videoStream() const
{
    Q_D(const QAVPlayer);
    return d->demuxer.videoStream();
}

void QAVPlayer::setVideoStream(const QAVStream &stream)
{
    Q_D(QAVPlayer);

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->demuxer.videoStream().index() << "->" << stream.index();
    if (d->demuxer.setVideoStream(stream))
        emit videoStreamChanged(stream);
}

QList<QAVStream> QAVPlayer::audioStreams() const
{
    Q_D(const QAVPlayer);
    return d->demuxer.audioStreams();
}

QAVStream QAVPlayer::audioStream() const
{
    Q_D(const QAVPlayer);
    return d->demuxer.audioStream();
}

void QAVPlayer::setAudioStream(const QAVStream &stream)
{
    Q_D(QAVPlayer);

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->demuxer.audioStream().index() << "->" << stream.index();
    if (d->demuxer.setAudioStream(stream))
        emit audioStreamChanged(stream);
}

QList<QAVStream> QAVPlayer::subtitleStreams() const
{
    Q_D(const QAVPlayer);
    return d->demuxer.subtitleStreams();
}

QAVStream QAVPlayer::subtitleStream() const
{
    Q_D(const QAVPlayer);
    return d->demuxer.subtitleStream();
}

void QAVPlayer::setSubtitleStream(const QAVStream &stream)
{
    Q_D(QAVPlayer);

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->demuxer.subtitleStream().index() << "->" << stream.index();
    if (d->demuxer.setSubtitleStream(stream))
        emit subtitleStreamChanged(stream);
}

QAVPlayer::State QAVPlayer::state() const
{
    Q_D(const QAVPlayer);
    QMutexLocker locker(&d->stateMutex);
    return d->state;
}

QAVPlayer::MediaStatus QAVPlayer::mediaStatus() const
{
    Q_D(const QAVPlayer);
    QMutexLocker locker(&d->stateMutex);
    return d->mediaStatus;
}

void QAVPlayer::play()
{
    Q_D(QAVPlayer);
    if (d->url.isEmpty() || d->currentError() == QAVPlayer::ResourceError)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__;
    if (d->setState(QAVPlayer::PlayingState)) {
        if (d->isEndOfFile()) {
            qCDebug(lcAVPlayer) << "Playing from beginning";
            seek(0);
        }
        d->setPendingMediaStatus(PlayingMedia);
    }
    d->wait(false);
    if (mediaStatus() != QAVPlayer::NoMedia)
        d->applyFilter();
}

void QAVPlayer::pause()
{
    Q_D(QAVPlayer);
    if (d->currentError() == QAVPlayer::ResourceError)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__;
    if (d->setState(QAVPlayer::PausedState)) {
        if (d->isEndOfFile()) {
            qCDebug(lcAVPlayer) << "Pausing from beginning";
            seek(0);
        }
        d->setPendingMediaStatus(PausingMedia);
        d->wait(false);
    } else {
        d->wait(true);
    }
    if (mediaStatus() != QAVPlayer::NoMedia)
        d->applyFilter();
}

void QAVPlayer::stop()
{
    Q_D(QAVPlayer);
    if (d->currentError() == QAVPlayer::ResourceError)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__;
    if (d->setState(QAVPlayer::StoppedState)) {
        d->setPendingMediaStatus(StoppingMedia);
        d->wait(false);
    } else {
        d->wait(true);
    }
    if (mediaStatus() != QAVPlayer::NoMedia)
        d->applyFilter();
}

void QAVPlayer::stepForward()
{
    Q_D(QAVPlayer);
    if (d->currentError() == QAVPlayer::ResourceError)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__;
    d->setState(QAVPlayer::PausedState);
    if (d->isEndOfFile()) {
        qCDebug(lcAVPlayer) << "Stepping from beginning";
        seek(0);
    }
    d->setPendingMediaStatus(SteppingMedia);
    d->wait(false);
    if (mediaStatus() != QAVPlayer::NoMedia)
        d->applyFilter();
}

void QAVPlayer::stepBackward()
{
    Q_D(QAVPlayer);
    if (d->currentError() == QAVPlayer::ResourceError)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__;
    d->setState(QAVPlayer::PausedState);
    const qint64 pos = d->pts() > 0 ? (d->pts() - videoFrameRate()) * 1000 : duration();
    seek(pos);
    d->setPendingMediaStatus(SteppingMedia);
    d->wait(false);
    if (mediaStatus() != QAVPlayer::NoMedia)
        d->applyFilter();
}

bool QAVPlayer::isSeekable() const
{
    return d_func()->seekable;
}

void QAVPlayer::seek(qint64 pos)
{
    Q_D(QAVPlayer);
    if ((duration() > 0 && pos > duration()) || d->currentError() == QAVPlayer::ResourceError)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << "pos:" << pos;
    {
        QMutexLocker locker(&d->positionMutex);
        d->pendingSeek = true;
        d->pendingPosition = pos / 1000.0;
    }

    d->setPendingMediaStatus(SeekingMedia);
    d->wait(false);
    if (mediaStatus() != QAVPlayer::NoMedia)
        d->applyFilter();
}

qint64 QAVPlayer::duration() const
{
    return d_func()->duration * 1000;
}

qint64 QAVPlayer::position() const
{
    Q_D(const QAVPlayer);

    QMutexLocker locker(&d->positionMutex);
    if (d->pendingSeek)
        return d->pendingPosition * 1000 + (d->pendingPosition < 0 ? duration() : 0);

    if (mediaStatus() == QAVPlayer::EndOfMedia)
        return duration();

    return d->pts() * 1000;
}

void QAVPlayer::setSpeed(qreal r)
{
    Q_D(QAVPlayer);

    {
        QMutexLocker locker(&d->speedMutex);
        if (qFuzzyCompare(d->speed, r))
            return;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->speed << "->" << r;
        d->speed = r;
    }
    emit speedChanged(r);
}

qreal QAVPlayer::speed() const
{
    Q_D(const QAVPlayer);

    QMutexLocker locker(&d->speedMutex);
    return d->speed;
}

double QAVPlayer::videoFrameRate() const
{
    return d_func()->videoFrameRate;
}

void QAVPlayer::setFilter(const QString &desc)
{
    Q_D(QAVPlayer);
    {
        QMutexLocker locker(&d->stateMutex);
        if (d->filterDesc == desc)
            return;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->filterDesc << "->" << desc;
        d->filterDesc = desc;
    }

    emit filterChanged(desc);
    if (mediaStatus() != QAVPlayer::NoMedia)
        d->applyFilter();
}

QString QAVPlayer::filter() const
{
    Q_D(const QAVPlayer);
    QMutexLocker locker(&d->stateMutex);
    return d->filterDesc;
}


bool QAVPlayer::isSynced() const
{
    Q_D(const QAVPlayer);
    return d->synced;
}

void QAVPlayer::setSynced(bool sync)
{
    Q_D(QAVPlayer);
    if (d->synced == sync)
        return;

    d->synced = sync;
    emit syncedChanged(sync);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QAVPlayer::State state)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (state) {
        case QAVPlayer::StoppedState:
            return dbg << "StoppedState";
        case QAVPlayer::PlayingState:
            return dbg << "PlayingState";
        case QAVPlayer::PausedState:
            return dbg << "PausedState";
        default:
            return dbg << QString(QLatin1String("UserType(%1)" )).arg(int(state)).toLatin1().constData();
    }
}

QDebug operator<<(QDebug dbg, QAVPlayer::MediaStatus status)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (status) {
        case QAVPlayer::NoMedia:
            return dbg << "NoMedia";
        case QAVPlayer::LoadedMedia:
            return dbg << "LoadedMedia";
        case QAVPlayer::EndOfMedia:
            return dbg << "EndOfMedia";
        case QAVPlayer::InvalidMedia:
            return dbg << "InvalidMedia";
        default:
            return dbg << QString(QLatin1String("UserType(%1)" )).arg(int(status)).toLatin1().constData();
    }
}

QDebug operator<<(QDebug dbg, PendingMediaStatus status)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (status) {
        case LoadingMedia:
            return dbg << "LoadingMedia";
        case PlayingMedia:
            return dbg << "PlayingMedia";
        case PausingMedia:
            return dbg << "PausingMedia";
        case StoppingMedia:
            return dbg << "StoppingMedia";
        case SteppingMedia:
            return dbg << "SteppingMedia";
        case SeekingMedia:
            return dbg << "SeekingMedia";
        case EndOfMedia:
            return dbg << "EndOfMedia";
        default:
            return dbg << QString(QLatin1String("UserType(%1)" )).arg(int(status)).toLatin1().constData();
    }
}

QDebug operator<<(QDebug dbg, QAVPlayer::Error err)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (err) {
        case QAVPlayer::NoError:
            return dbg << "NoError";
        case QAVPlayer::ResourceError:
            return dbg << "ResourceError";
        case QAVPlayer::FilterError:
            return dbg << "FilterError";
        default:
            return dbg << QString(QLatin1String("UserType(%1)" )).arg(int(err)).toLatin1().constData();
    }
}
#endif

Q_DECLARE_METATYPE(PendingMediaStatus)

QT_END_NAMESPACE
