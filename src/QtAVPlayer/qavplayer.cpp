/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"
#include "qavdemuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavvideoframe.h"
#include "qavaudioframe.h"
#include "qavpacketqueue_p.h"
#include <QtConcurrent/qtconcurrentrun.h>
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
    {
        threadPool.setMaxThreadCount(4);
    }

    void setMediaStatus(QAVPlayer::MediaStatus status);
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

    void terminate();

    void doWait();
    void wait(bool v);
    void doLoad(const QUrl &url);
    void doDemux();
    void doPlayVideo();
    void doPlayAudio();

    template <class T>
    void dispatch(T fn);

    QAVPlayer *q_ptr = nullptr;
    QUrl url;
    QAVPlayer::MediaStatus mediaStatus = QAVPlayer::NoMedia;
    QList<PendingMediaStatus> pendingMediaStatuses;
    QAVPlayer::State state = QAVPlayer::StoppedState;
    mutable QMutex stateMutex;

    bool seekable = false;
    qreal speed = 1.0;
    mutable QMutex speedMutex;
    double videoFrameRate = 0.0;

    QAVPlayer::Error error = QAVPlayer::NoError;
    QString errorString;

    double duration = 0;
    double pendingPosition = -1;
    mutable QMutex positionMutex;

    int videoStream = -1;
    int audioStream = -1;
    int subtitleStream = -1;
    QMutex streamsMutex;

    QAVDemuxer demuxer;

    QThreadPool threadPool;
    QFuture<void> loaderFuture;
    QFuture<void> demuxerFuture;

    QFuture<void> videoPlayFuture;
    QAVPacketQueue videoQueue;

    QFuture<void> audioPlayFuture;
    QAVPacketQueue audioQueue;

    bool quit = 0;
    bool isWaiting = false;
    mutable QMutex waitMutex;
    QWaitCondition waitCond;
    bool eof = false;
};

static QString err_str(int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));

    return QString::fromUtf8(errbuf_ptr);
}

void QAVPlayerPrivate::setMediaStatus(QAVPlayer::MediaStatus status)
{
    {
        QMutexLocker locker(&stateMutex);
        if (mediaStatus == status)
            return;

        qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << mediaStatus << "->" << status;
        mediaStatus = status;
    }

    emit q_ptr->mediaStatusChanged(status);
}

void QAVPlayerPrivate::setPendingMediaStatus(PendingMediaStatus status)
{
    QMutexLocker locker(&stateMutex);
    pendingMediaStatuses.push_back(status);
    qCDebug(lcAVPlayer) << __FUNCTION__ << ":"<< mediaStatus << "->" << pendingMediaStatuses;
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
    return pendingPosition >= 0;
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

template <class T>
void QAVPlayerPrivate::dispatch(T fn)
{
    QMetaObject::invokeMethod(q_ptr, fn, nullptr);
}

void QAVPlayerPrivate::setError(QAVPlayer::Error err, const QString &str)
{
    Q_Q(QAVPlayer);
    if (error == err)
        return;

    qWarning() << "Error:" << url << ":"<< str;
    error = err;
    errorString = str;
    emit q->errorOccurred(err, str);
    setMediaStatus(QAVPlayer::InvalidMedia);
}

void QAVPlayerPrivate::terminate()
{
    qCDebug(lcAVPlayer) << __FUNCTION__;
    setState(QAVPlayer::StoppedState);
    demuxer.abort();
    quit = true;
    wait(false);
    videoFrameRate = 0.0;
    videoQueue.clear();
    videoQueue.abort();
    audioQueue.clear();
    audioQueue.abort();
    loaderFuture.waitForFinished();
    demuxerFuture.waitForFinished();
    videoPlayFuture.waitForFinished();
    audioPlayFuture.waitForFinished();
    pendingPosition = -1;
    pendingMediaStatuses.clear();
    setDuration(0);
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
        if (!pendingMediaStatuses.isEmpty())
            pendingMediaStatuses.removeFirst();
    }

    if (pendingMediaStatuses.isEmpty()) {
        videoQueue.wake(false);
        audioQueue.wake(false);
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
}

void QAVPlayerPrivate::doLoad(const QUrl &url)
{
    demuxer.abort(false);
    demuxer.unload();
    int ret = demuxer.load(url);
    if (ret < 0) {
        dispatch([this, ret] { setError(QAVPlayer::ResourceError, err_str(ret)); });
        return;
    }

    if (demuxer.videoStream() < 0 && demuxer.audioStream() < 0) {
        dispatch([this] { setError(QAVPlayer::ResourceError, QLatin1String("No codecs found")); });
        return;
    }

    dispatch([this] {
        qCDebug(lcAVPlayer) << "[" << this->url << "]: Loaded, seekable:" << demuxer.seekable() << ", duration:" << demuxer.duration();
        setSeekable(demuxer.seekable());
        setDuration(demuxer.duration());
        setVideoFrameRate(demuxer.frameRate());
        q_ptr->setVideoStream(demuxer.videoStreamIndex());
        q_ptr->setAudioStream(demuxer.audioStreamIndex());
        step(false);
    });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    demuxerFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doDemux);
    if (q_ptr->hasVideo())
        videoPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayVideo);
    if (q_ptr->hasAudio())
        audioPlayFuture = QtConcurrent::run(&threadPool, this, &QAVPlayerPrivate::doPlayAudio);
#else
    demuxerFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doDemux, this);
    if (q_ptr->hasVideo())
        videoPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayVideo, this);
    if (q_ptr->hasAudio())
        audioPlayFuture = QtConcurrent::run(&threadPool, &QAVPlayerPrivate::doPlayAudio, this);
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
            if (pendingPosition >= 0) {
                const double pos = pendingPosition;
                locker.unlock();
                qCDebug(lcAVPlayer) << "Seeking to pos:" << pos * 1000;
                const int ret = demuxer.seek(pos);
                if (ret >= 0) {
                    videoQueue.clear();
                    audioQueue.clear();
                    qCDebug(lcAVPlayer) << "Waiting video thread finished processing packets";
                    videoQueue.waitForEmpty();
                    qCDebug(lcAVPlayer) << "Waiting audio thread finished processing packets";
                    audioQueue.waitForEmpty();
                    qCDebug(lcAVPlayer) << "Start reading packets from" << pos * 1000;
                } else {
                    qWarning() << "Could not seek:" << err_str(ret);
                }
                locker.relock();
                if (qFuzzyCompare(pendingPosition, pos))
                    pendingPosition = -1;
            }
        }

        auto packet = demuxer.read();
        if (packet) {
            endOfFile(false);
            if (packet.streamIndex() == demuxer.videoStream())
                videoQueue.enqueue(packet);
            else if (packet.streamIndex() == demuxer.audioStream())
                audioQueue.enqueue(packet);
        } else {
            if (demuxer.eof() && videoQueue.isEmpty() && audioQueue.isEmpty() && !isEndOfFile()) {
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

void QAVPlayerPrivate::doPlayVideo()
{
    videoQueue.setFrameRate(demuxer.frameRate());

    while (!quit) {
        doWait();
        QAVVideoFrame frame = videoQueue.sync(q_ptr->speed(), audioQueue.pts());
        if (frame) {
            emit q_ptr->videoFrame(frame);
            videoQueue.pop();
        }
        step(frame);
    }

    videoQueue.clear();
    setMediaStatus(QAVPlayer::NoMedia);
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

void QAVPlayerPrivate::doPlayAudio()
{
    const bool hasVideo = q_ptr->hasVideo();

    while (!quit) {
        doWait();
        const qreal currSpeed = q_ptr->speed();
        QAVAudioFrame frame = audioQueue.sync(currSpeed);
        if (frame) {
            frame.frame()->sample_rate *= currSpeed;
            emit q_ptr->audioFrame(frame);
            audioQueue.pop();
        }

        if (!hasVideo)
            step(frame);
    }

    audioQueue.clear();
    if (!hasVideo)
        setMediaStatus(QAVPlayer::NoMedia);
    qCDebug(lcAVPlayer) << __FUNCTION__ << "finished";
}

QAVPlayer::QAVPlayer(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVPlayerPrivate(this))
{
    qRegisterMetaType<QAVAudioFrame>();
    qRegisterMetaType<QAVVideoFrame>();
    qRegisterMetaType<State>();
    qRegisterMetaType<MediaStatus>();
}

QAVPlayer::~QAVPlayer()
{
    Q_D(QAVPlayer);
    d->terminate();
}

void QAVPlayer::setSource(const QUrl &url)
{
    Q_D(QAVPlayer);
    if (d->url == url)
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << url;
    d->terminate();
    d->url = url;
    emit sourceChanged(url);
    d->wait(true);
    d->quit = false;
    if (d->url.isEmpty())
        return;

    d->setPendingMediaStatus(LoadingMedia);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d->loaderFuture = QtConcurrent::run(&d->threadPool, d, &QAVPlayerPrivate::doLoad, d->url);
#else
    d->loaderFuture = QtConcurrent::run(&d->threadPool, &QAVPlayerPrivate::doLoad, d, d->url);
#endif
}

QUrl QAVPlayer::source() const
{
    return d_func()->url;
}

bool QAVPlayer::hasAudio() const
{
    return audioStreamsCount() > 0;
}

bool QAVPlayer::hasVideo() const
{
    return videoStreamsCount() > 0;
}

int QAVPlayer::videoStreamsCount() const
{
    return d_func()->demuxer.videoStreams().size();
}

int QAVPlayer::videoStream() const
{
    return d_func()->videoStream;
}

void QAVPlayer::setVideoStream(int stream)
{
    Q_D(QAVPlayer);

    if (stream < 0 || stream == d->videoStream || stream >= videoStreamsCount())
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->videoStream << "->" << stream;
    d->videoStream = stream;
    d->demuxer.setVideoStreamIndex(stream);
    emit videoStreamChanged(stream);
}

int QAVPlayer::audioStreamsCount() const
{
    return d_func()->demuxer.audioStreams().size();
}

int QAVPlayer::audioStream() const
{
    return d_func()->audioStream;
}

void QAVPlayer::setAudioStream(int stream)
{
    Q_D(QAVPlayer);

    if (stream < 0 || stream == d->audioStream || stream >= audioStreamsCount())
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << d->audioStream << "->" << stream;
    d->audioStream = stream;
    d->demuxer.setAudioStreamIndex(stream);
    emit audioStreamChanged(stream);
}

int QAVPlayer::subtitleStreamsCount() const
{
    return d_func()->demuxer.subtitleStreams().size();
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
    if (d->url.isEmpty() || mediaStatus() == QAVPlayer::InvalidMedia)
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
}

void QAVPlayer::pause()
{
    Q_D(QAVPlayer);
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
}

void QAVPlayer::stop()
{
    Q_D(QAVPlayer);
    qCDebug(lcAVPlayer) << __FUNCTION__;
    if (d->setState(QAVPlayer::StoppedState)) {
        d->setPendingMediaStatus(StoppingMedia);
        d->wait(false);
    } else {
        d->wait(true);
    }
}

void QAVPlayer::stepForward()
{
    Q_D(QAVPlayer);
    qCDebug(lcAVPlayer) << __FUNCTION__;
    d->setState(QAVPlayer::PausedState);
    if (d->isEndOfFile()) {
        qCDebug(lcAVPlayer) << "Stepping from beginning";
        seek(0);
    }
    d->setPendingMediaStatus(SteppingMedia);
    d->wait(false);
}

bool QAVPlayer::isSeekable() const
{
    return d_func()->seekable;
}

void QAVPlayer::seek(qint64 pos)
{
    Q_D(QAVPlayer);
    if (pos < 0 || (duration() > 0 && pos > duration()))
        return;

    qCDebug(lcAVPlayer) << __FUNCTION__ << ":" << "pos:" << pos;
    {
        QMutexLocker locker(&d->positionMutex);
        d->pendingPosition = pos / 1000.0;
    }

    d->setPendingMediaStatus(SeekingMedia);
    d->wait(false);
}

qint64 QAVPlayer::duration() const
{
    return d_func()->duration * 1000;
}

qint64 QAVPlayer::position() const
{
    Q_D(const QAVPlayer);

    QMutexLocker locker(&d->positionMutex);
    if (d->pendingPosition >= 0)
        return d->pendingPosition * 1000;

    if (mediaStatus() == QAVPlayer::EndOfMedia)
        return duration();

    const double pts = hasVideo() ? d->videoQueue.pts() : d->audioQueue.pts();
    return pts * 1000;
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

QAVPlayer::Error QAVPlayer::error() const
{
    return d_func()->error;
}

QString QAVPlayer::errorString() const
{
    return d_func()->errorString;
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
#endif

QT_END_NAMESPACE
