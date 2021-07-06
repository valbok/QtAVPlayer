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

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

class QAVPlayerPrivate
{
    Q_DECLARE_PUBLIC(QAVPlayer)
public:
    QAVPlayerPrivate(QAVPlayer *q)
        : q_ptr(q)
    {
    }

    void setMediaStatus(QAVPlayer::MediaStatus status);
    void setState(QAVPlayer::State s);
    void setSeekable(bool seekable);
    void setError(QAVPlayer::Error err, const QString &str);
    void setDuration(double d);

    void terminate();

    void doWait();
    bool setWait(bool v);
    void doLoad(const QUrl &url);
    void doDemux();
    void doPlayVideo();
    void doPlayAudio();

    template <class T>
    void dispatch(T fn);

    QAVPlayer *q_ptr = nullptr;
    QUrl url;
    QAVPlayer::MediaStatus mediaStatus = QAVPlayer::NoMedia;
    QAVPlayer::State state = QAVPlayer::StoppedState;

    bool seekable = false;
    qreal speed = 1.0;
    mutable QMutex speedMutex;

    QAVPlayer::Error error = QAVPlayer::NoError;
    QString errorString;

    double duration = 0;
    double position = 0;
    double pendingPosition = -1;
    mutable QMutex pendingPositionMutex;

    bool pendingPlay = false;

    int videoStream = -1;
    int audioStream = -1;
    int subtitleStream = -1;

    QAVDemuxer demuxer;

    QFuture<void> loaderFuture;
    QFuture<void> demuxerFuture;

    QFuture<void> videoPlayFuture;
    QAVPacketQueue videoQueue;

    QFuture<void> audioPlayFuture;
    QAVPacketQueue audioQueue;
    double audioPts = 0;

    bool quit = 0;
    bool wait = false;
    QMutex waitMutex;
    QWaitCondition waitCond;
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
    Q_Q(QAVPlayer);
    if (mediaStatus == status)
        return;

    mediaStatus = status;
    emit q->mediaStatusChanged(status);
    if (pendingPlay && status == QAVPlayer::LoadedMedia)
        q->play();
}

void QAVPlayerPrivate::setState(QAVPlayer::State s)
{
    Q_Q(QAVPlayer);
    if (state == s)
        return;

    state = s;
    emit q->stateChanged(s);
}

void QAVPlayerPrivate::setSeekable(bool s)
{
    Q_Q(QAVPlayer);
    if (seekable == s)
        return;

    seekable = s;
    emit q->seekableChanged(seekable);
}

void QAVPlayerPrivate::setDuration(double d)
{
    Q_Q(QAVPlayer);
    if (qFuzzyCompare(duration, d))
        return;

    duration = d;
    emit q->durationChanged(q->duration());
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
    setState(QAVPlayer::StoppedState);
    demuxer.abort();
    quit = true;
    pendingPlay = false;
    setWait(false);
    videoQueue.clear();
    videoQueue.wakeAll();
    audioQueue.clear();
    audioQueue.wakeAll();
    loaderFuture.waitForFinished();
    demuxerFuture.waitForFinished();
    videoPlayFuture.waitForFinished();
    audioPlayFuture.waitForFinished();
}

void QAVPlayerPrivate::doWait()
{
    QMutexLocker lock(&waitMutex);
    if (wait)
        waitCond.wait(&waitMutex);
}

bool QAVPlayerPrivate::setWait(bool v)
{
    bool result = false;
    {
        QMutexLocker locker(&waitMutex);
        result = v != wait;
        wait = v;
    }

    if (!v)
        waitCond.wakeAll();

    return result;
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

    double d = demuxer.duration();
    bool seekable = demuxer.seekable();
    int vs = demuxer.videoStream();
    int as = demuxer.audioStream();
    int ss = demuxer.subtitleStream();
    dispatch([this, d, seekable, vs, as, ss] {
        setSeekable(seekable);
        setDuration(d);
        setMediaStatus(QAVPlayer::LoadedMedia);
        videoStream = vs;
        audioStream = as;
        subtitleStream = ss;
    });

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    demuxerFuture = QtConcurrent::run(this, &QAVPlayerPrivate::doDemux);
    videoPlayFuture = QtConcurrent::run(this, &QAVPlayerPrivate::doPlayVideo);
    audioPlayFuture = QtConcurrent::run(this, &QAVPlayerPrivate::doPlayAudio);
#else
    demuxerFuture = QtConcurrent::run(&QAVPlayerPrivate::doDemux, this);
    videoPlayFuture = QtConcurrent::run(&QAVPlayerPrivate::doPlayVideo, this);
    audioPlayFuture = QtConcurrent::run(&QAVPlayerPrivate::doPlayAudio, this);
#endif
}

void QAVPlayerPrivate::doDemux()
{
    const int maxQueueBytes = 15 * 1024 * 1024;
    QMutex waiterMutex;
    QWaitCondition waiter;

    while (!quit) {
        doWait();
        if (videoQueue.bytes() + audioQueue.bytes() > maxQueueBytes
            || (videoQueue.enough() && audioQueue.enough()))
        {
            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
            continue;
        }

        {
            QMutexLocker locker(&pendingPositionMutex);
            if (pendingPosition >= 0) {
                int ret = demuxer.seek(pendingPosition);
                if (ret >= 0) {
                    videoQueue.clear();
                    audioQueue.clear();
                } else {
                    qWarning() << "Could not seek:" << err_str(ret);
                }
            }
            pendingPosition = -1;
        }

        auto packet = demuxer.read();
        if (!packet) {
            if (demuxer.eof() && videoQueue.isEmpty() && audioQueue.isEmpty()) {
                dispatch([this] {
                    q_ptr->stop();
                    setMediaStatus(QAVPlayer::EndOfMedia);
                });
            }

            QMutexLocker locker(&waiterMutex);
            waiter.wait(&waiterMutex, 10);
            continue;
        }

        if (packet.streamIndex() == demuxer.videoStream()) {
            videoQueue.enqueue(packet);
        } else if (packet.streamIndex() == demuxer.audioStream()) {
            audioQueue.enqueue(packet);
        }
    }
}

void QAVPlayerPrivate::doPlayVideo()
{
    videoQueue.setFrameRate(demuxer.frameRate());

    while (!quit) {
        doWait();
        QAVVideoFrame frame = videoQueue.sync(q_ptr->speed(), audioQueue.pts());
        if (!frame)
            continue;

        dispatch([this, frame] {
            const bool publish = state == QAVPlayer::PausedState || state == QAVPlayer::StoppedState
                               ? setWait(true)
                               : true;
            if (publish)
                emit q_ptr->videoFrame(frame);
            position = frame.pts();
        });

        videoQueue.pop();
    }

    videoQueue.clear();
}

void QAVPlayerPrivate::doPlayAudio()
{
    QAVAudioFrame frame;

    while (!quit) {
        doWait();
        frame = audioQueue.sync(q_ptr->speed());
        if (!frame)
            continue;

        dispatch([this, frame] {
            frame.frame()->sample_rate *= q_ptr->speed();
            emit q_ptr->audioFrame(frame);
            if (!q_ptr->hasVideo())
                position = frame.pts();
        });

        audioQueue.pop();
    }

    audioQueue.clear();
}

QAVPlayer::QAVPlayer(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVPlayerPrivate(this))
{
    const int maxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
    if (maxThreadCount < 4)
        qWarning() << "Max thread count is to low:" << maxThreadCount;
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

    d->terminate();
    d->url = url;
    emit sourceChanged(url);
    if (d->url.isEmpty()) {
        d->setMediaStatus(QAVPlayer::NoMedia);
        d->setDuration(0);
        d->position = 0;
        return;
    }

    d->setWait(true);
    d->quit = false;
    d->setMediaStatus(QAVPlayer::LoadingMedia);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d->loaderFuture = QtConcurrent::run(d, &QAVPlayerPrivate::doLoad, d->url);
#else
    d->loaderFuture = QtConcurrent::run(&QAVPlayerPrivate::doLoad, d, d->url);
#endif
}

QUrl QAVPlayer::source() const
{
    return d_func()->url;
}

bool QAVPlayer::hasAudio() const
{
    return d_func()->audioStream >= 0;
}

bool QAVPlayer::hasVideo() const
{
    return d_func()->videoStream >= 0;
}

QAVPlayer::State QAVPlayer::state() const
{
    Q_D(const QAVPlayer);
    return d->state;
}

QAVPlayer::MediaStatus QAVPlayer::mediaStatus() const
{
    return d_func()->mediaStatus;
}

void QAVPlayer::play()
{
    Q_D(QAVPlayer);
    if (d->url.isEmpty() || d->mediaStatus == QAVPlayer::InvalidMedia)
        return;

    d->setState(QAVPlayer::PlayingState);
    if (d->mediaStatus == QAVPlayer::EndOfMedia) {
        QMutexLocker locker(&d->pendingPositionMutex);
        if (d->pendingPosition < 0) {
            locker.unlock();
            seek(0);
        }
        d->setMediaStatus(QAVPlayer::LoadedMedia);
    } else if (d->mediaStatus != QAVPlayer::LoadedMedia) {
        d->pendingPlay = true;
        return;
    }

    d->setWait(false);
    d->pendingPlay = false;
}

void QAVPlayer::pause()
{
    Q_D(QAVPlayer);
    d->setState(QAVPlayer::PausedState);
    d->setWait(!hasVideo());
}

void QAVPlayer::stop()
{
    Q_D(QAVPlayer);
    d->setState(QAVPlayer::StoppedState);
    d->setWait(true);
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

    {
        QMutexLocker lock(&d->pendingPositionMutex);
        d->pendingPosition = pos / 1000.0;
    }

    d->setMediaStatus(QAVPlayer::LoadedMedia);
    d->setWait(false);
}

qint64 QAVPlayer::duration() const
{
    return d_func()->duration * 1000;
}

qint64 QAVPlayer::position() const
{
    Q_D(const QAVPlayer);

    if (d->mediaStatus == QAVPlayer::EndOfMedia)
        return duration();

    {
        QMutexLocker lock(&d->pendingPositionMutex);
        if (d->pendingPosition >= 0)
            return d->pendingPosition * 1000;
    }

    return d->position * 1000;
}

void QAVPlayer::setSpeed(qreal r)
{
    Q_D(QAVPlayer);

    {
        QMutexLocker locker(&d->speedMutex);
        if (qFuzzyCompare(d->speed, r))
            return;

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

QAVPlayer::Error QAVPlayer::error() const
{
    return d_func()->error;
}

QString QAVPlayer::errorString() const
{
    return d_func()->errorString;
}

QT_END_NAMESPACE
