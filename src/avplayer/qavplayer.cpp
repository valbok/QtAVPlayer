/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#include "qavplayer.h"
#include "qavaudiooutput.h"
#include "qavdemuxer_p.h"
#include "qavvideocodec_p.h"
#include "qavaudiocodec_p.h"
#include "qavvideoframe_p.h"
#include "qavaudioframe_p.h"
#include "qavpacketqueue_p.h"

#include <QAbstractVideoSurface>
#include <qtconcurrentrun.h>

QT_BEGIN_NAMESPACE

class QAVPlayerPrivate
{
    Q_DECLARE_PUBLIC(QAVPlayer)
public:
    QAVPlayerPrivate(QAVPlayer *q)
        : q_ptr(q)
    {
    }

    void setMediaStatus(QMediaPlayer::MediaStatus status);
    void setState(QMediaPlayer::State s);
    void setSeekable(bool seekable);
    void setError(QMediaPlayer::Error err, const QString &str);
    void setDuration(double d);
    void updatePosition(double p);

    bool isAudioAvailable() const;
    bool isVideoAvailable() const;
    void terminate();
    void doWait();
    void doEndOfMedia();

    void doLoad(const QUrl &url);
    void doDemux();
    void doPlayVideo();
    void doFlushVideo();
    void doPlayAudio();

    template <class T>
    void call(T fn);

    QAVPlayer *q_ptr = nullptr;
    QUrl url;
    QAbstractVideoSurface *surface = nullptr;
    QMediaPlayer::MediaStatus mediaStatus = QMediaPlayer::NoMedia;
    QMediaPlayer::State state = QMediaPlayer::StoppedState;
    bool seekable = false;
    int volume = 100;
    bool muted = false;
    qreal speed = 1.0;

    QMediaPlayer::Error error = QMediaPlayer::NoError;
    QString errorString;

    double duration = 0;
    double position = 0;
    double pendingPosition = -1;
    bool pendingPlay = false;

    QAVDemuxer demuxer;

    QFuture<void> loaderFuture;
    QFuture<void> demuxerFuture;

    QFuture<void> videoPlayFuture;
    QAVPacketQueue videoQueue;

    QFuture<void> audioPlayFuture;
    QAVPacketQueue audioQueue;
    double audioPts = 0;

    QScopedPointer<QAVAudioOutput> audioOutput;

    bool quit = 0;
    bool wait = false;
    QMutex waitMutex;
    QWaitCondition waitCond;
    mutable QMutex positionMutex;

    std::function<void(const QVideoFrame &data)> vo;
    std::function<void(const QAudioBuffer &data)> ao;
};

static QString err_str(int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));

    return QString::fromUtf8(errbuf_ptr);
}

void QAVPlayerPrivate::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    Q_Q(QAVPlayer);
    if (mediaStatus == status)
        return;

    mediaStatus = status;
    emit q->mediaStatusChanged(status);
    if (pendingPlay && status == QMediaPlayer::LoadedMedia)
        q->play();
}

void QAVPlayerPrivate::setState(QMediaPlayer::State s)
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

void QAVPlayerPrivate::updatePosition(double p)
{
    QMutexLocker lock(&positionMutex);
    position = p;
}

template <class T>
void QAVPlayerPrivate::call(T fn)
{
    QMetaObject::invokeMethod(q_ptr, fn, nullptr);
}

void QAVPlayerPrivate::setError(QMediaPlayer::Error err, const QString &str)
{
    Q_Q(QAVPlayer);
    if (error == err)
        return;

    qWarning() << "Error:" << url << ":"<< str;
    error = err;
    errorString = str;
    emit q->errorOccurred(err, str);
    setMediaStatus(QMediaPlayer::InvalidMedia);
}

bool QAVPlayerPrivate::isAudioAvailable() const
{
    return demuxer.audioStream() >= 0;
}

bool QAVPlayerPrivate::isVideoAvailable() const
{
    return demuxer.videoStream() >= 0;
}

void QAVPlayerPrivate::terminate()
{
    setState(QMediaPlayer::StoppedState);
    demuxer.abort();
    quit = true;
    pendingPlay = false;
    wait = false;
    waitCond.wakeAll();
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
        waitCond.wait(&waitMutex, 5000);
}

void QAVPlayerPrivate::doFlushVideo()
{
    call([this] { if (surface) surface->present({}); });
}

void QAVPlayerPrivate::doLoad(const QUrl &url)
{
    demuxer.abort(false);
    demuxer.unload();
    int ret = demuxer.load(url);
    if (ret < 0) {
        call([this, ret] { setError(QMediaPlayer::ResourceError, err_str(ret)); });
        return;
    }

    if (demuxer.videoStream() < 0 && demuxer.audioStream() < 0) {
        call([this] { setError(QMediaPlayer::ResourceError, QLatin1String("No codecs found")); });
        return;
    }

    // Negotiate pixel formats here.
    if (demuxer.videoCodec())
        demuxer.videoCodec()->setVideoSurface(surface);

    double d = demuxer.duration();
    bool seekable = demuxer.seekable();
    call([this, seekable, d] {
        setSeekable(seekable);
        setDuration(d);
        setMediaStatus(QMediaPlayer::LoadedMedia);
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
    QMutex mutex;
    QWaitCondition waiter;

    while (!quit) {
        doWait();
        if (videoQueue.bytes() + audioQueue.bytes() > maxQueueBytes
            || (videoQueue.enough() && audioQueue.enough()))
        {
            QMutexLocker locker(&mutex);
            waiter.wait(&mutex, 10);
            continue;
        }

        positionMutex.lock();
        if (pendingPosition >= 0) {
            int ret = demuxer.seek(pendingPosition);
            if (ret >= 0) {
                videoQueue.clear();
                audioQueue.clear();
            } else {
                qWarning() << "Could not seek:" << err_str(ret);
            }
            pendingPosition = -1;
        }
        positionMutex.unlock();

        auto packet = demuxer.read();
        if (!packet) {
            if (demuxer.eof() && videoQueue.isEmpty() && audioQueue.isEmpty())
                doEndOfMedia();

            QMutexLocker locker(&mutex);
            waiter.wait(&mutex, 10);
            continue;
        }

        if (packet.streamIndex() == demuxer.videoStream())
            videoQueue.enqueue(packet);
        else if (packet.streamIndex() == demuxer.audioStream())
            audioQueue.enqueue(packet);
    }
}

void QAVPlayerPrivate::doEndOfMedia()
{
    Q_Q(QAVPlayer);
    doFlushVideo();
    call([this, q] {
        q->stop();
        setMediaStatus(QMediaPlayer::EndOfMedia);
    });
}

void QAVPlayerPrivate::doPlayVideo()
{
    videoQueue.setFrameRate(demuxer.frameRate());

    while (!quit) {
        doWait();
        QAVVideoFrame frame = videoQueue.sync(speed, audioQueue.pts());
        if (!frame)
            continue;

        QVideoFrame videoFrame = frame;
        call([this, videoFrame] {
            if (vo) {
                vo(videoFrame);
                return;
            }

            if (!surface)
                return;
            if (!surface->isActive())
                surface->start({videoFrame.size(), videoFrame.pixelFormat(), videoFrame.handleType()});
            if (surface->isActive())
                surface->present(videoFrame);
        });

        videoQueue.pop();
        updatePosition(frame.pts());
    }

    doFlushVideo();
    videoQueue.clear();
}

void QAVPlayerPrivate::doPlayAudio()
{
    QAVAudioFrame frame;
    QAudioFormat format;

    while (!quit) {
        doWait();
        frame = audioQueue.sync(speed);
        if (!frame)
            continue;

        if (!format.isValid()) {
            format = frame.codec()->audioFormat();
            if (format.sampleType() != QAudioFormat::SignedInt) {
                format.setSampleSize(16);
                format.setSampleType(QAudioFormat::SignedInt);
            }

            if (!ao)
                audioOutput.reset(new QAVAudioOutput);
        }

        if (!muted) {
            frame.frame()->sample_rate *= speed;
            auto data = frame.data(format);
            QAudioBuffer buf(data, format);
            if (ao) {
                call([this, buf] { ao(buf); });
            } else {
                audioOutput->setVolume(volume / 100.0);
                audioOutput->play(buf);
            }
        }

        audioQueue.pop();
        if (!isVideoAvailable())
            updatePosition(frame.pts());
    }

    audioOutput.reset();
    audioQueue.clear();
}

QAVPlayer::QAVPlayer(QObject *parent)
    : QObject(parent)
    , d_ptr(new QAVPlayerPrivate(this))
{
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
        d->setMediaStatus(QMediaPlayer::NoMedia);
        d->setDuration(0);
        d->updatePosition(0);
        return;
    }

    d->quit = false;
    d->setMediaStatus(QMediaPlayer::LoadingMedia);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    d->loaderFuture = QtConcurrent::run(d, &QAVPlayerPrivate::doLoad, d->url);
#else
    d->loaderFuture = QtConcurrent::run(&QAVPlayerPrivate::doLoad, d, d->url);
#endif
    QMutexLocker locker(&d->waitMutex);
    d->wait = true;
}

QUrl QAVPlayer::source() const
{
    return d_func()->url;
}

bool QAVPlayer::isAudioAvailable() const
{
    return d_func()->isAudioAvailable();
}

bool QAVPlayer::isVideoAvailable() const
{
    return d_func()->isVideoAvailable();
}

void QAVPlayer::vo(QAbstractVideoSurface *surface)
{
    Q_D(QAVPlayer);
    d->surface = surface;
    d->vo = nullptr;
}

void QAVPlayer::vo(std::function<void(const QVideoFrame &data)> f)
{
    Q_D(QAVPlayer);
    d->surface = nullptr;
    d->vo = f;
}

void QAVPlayer::ao(std::function<void(const QAudioBuffer &buf)> f)
{
    d_func()->ao = f;
}

QMediaPlayer::State QAVPlayer::state() const
{
    return d_func()->state;
}

QMediaPlayer::MediaStatus QAVPlayer::mediaStatus() const
{
    return d_func()->mediaStatus;
}

void QAVPlayer::play()
{
    Q_D(QAVPlayer);
    if (d->url.isEmpty() || d->mediaStatus == QMediaPlayer::InvalidMedia)
        return;

    d->setState(QMediaPlayer::PlayingState);
    if (d->mediaStatus == QMediaPlayer::EndOfMedia) {
        QMutexLocker locker(&d->positionMutex);
        if (d->pendingPosition < 0) {
            locker.unlock();
            seek(0);
        }
        d->setMediaStatus(QMediaPlayer::LoadedMedia);
    } else if (d->mediaStatus != QMediaPlayer::LoadedMedia) {
        d->pendingPlay = true;
        return;
    }

    QMutexLocker locker(&d->waitMutex);
    if (!d->wait)
        return;

    d->wait = false;
    d->waitCond.wakeAll();
    d->pendingPlay = false;
}

void QAVPlayer::pause()
{
    Q_D(QAVPlayer);
    d->setState(QMediaPlayer::PausedState);
    QMutexLocker locker(&d->waitMutex);
    d->wait = true;
}

void QAVPlayer::stop()
{
    Q_D(QAVPlayer);
    d->setState(QMediaPlayer::StoppedState);
    QMutexLocker locker(&d->waitMutex);
    d->wait = true;
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

    QMutexLocker lock(&d->positionMutex);
    d->pendingPosition = pos / 1000.0;
}

qint64 QAVPlayer::duration() const
{
    return d_func()->duration * 1000;
}

qint64 QAVPlayer::position() const
{
    Q_D(const QAVPlayer);
    QMutexLocker lock(&d->positionMutex);
    if (d->pendingPosition >= 0)
        return d->pendingPosition * 1000;
    if (d->mediaStatus == QMediaPlayer::EndOfMedia)
        return duration();
    return d->position * 1000;
}

void QAVPlayer::setVolume(int volume)
{
    Q_D(QAVPlayer);
    if (d->volume == volume || volume < 0 || volume > 100)
        return;

    d->volume = volume;
    emit volumeChanged(volume);
}

int QAVPlayer::volume() const
{
    return d_func()->volume;
}

void QAVPlayer::setMuted(bool m)
{
    Q_D(QAVPlayer);
    if (d->muted == m)
        return;

    d->muted = m;
    emit mutedChanged(m);
}

bool QAVPlayer::isMuted() const
{
    return d_func()->muted;
}

void QAVPlayer::setSpeed(qreal r)
{
    Q_D(QAVPlayer);
    if (qFuzzyCompare(d->speed, r))
        return;

    d->speed = r;
    emit speedChanged(r);
}

qreal QAVPlayer::speed() const
{
    return d_func()->speed;
}

QMediaPlayer::Error QAVPlayer::error() const
{
    return d_func()->error;
}

QString QAVPlayer::errorString() const
{
    return d_func()->errorString;
}

QT_END_NAMESPACE
