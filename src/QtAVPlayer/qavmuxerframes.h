/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVMUXERFRAMES_H
#define QAVMUXERFRAMES_H

#include <QtAVPlayer/qavmuxer.h>
#include <QSize>

QT_BEGIN_NAMESPACE

/**
 * Re-encodes the frames using specific encoder
 */
class QAVMuxerFramesPrivate;
class QAVMuxerFrames : public QAVMuxer
{
public:
    /**
     * Defines properties for encoder.
     * Used to initialize AVCodecContext.
     */
    class EncoderStream
    {
    public:
        EncoderStream(const QAVStream &stream);
        const QAVStream &stream() const;

        // Sets the size of the frames
        void setSize(const QSize &size);

        // Returns the size of the frames
        const QSize &size() const;

    private:
        QAVStream m_stream;
        QSize m_size;
    };

    QAVMuxerFrames();
    ~QAVMuxerFrames() override;

    /**
     * Loads the encoder based on parsed streams, format is negotiated from filename.
     * It uses AVCodecContext from the stream's codec to initialize the encoder.
     */
    int load(const QList<QAVStream> &streams, const QString &filename);

    /**
     * Loads the encoder based on parsed streams and params from EncoderStream.
     * If params are not set, the stream's codec AVCodecContext is used instead.
     */
    int load(const QList<EncoderStream> &streams, const QString &filename);

    /**
     * Sets desired output codec. E.g h264_nvenc for h264_cuvid decoder.
     * Uses avcodec_find_encoder_by_name() internally.
     * Should be called before load().
     */
    void setOutputVideoCodec(const QString &name);
    QString outputVideoCodec() const;

    // Adds the frame to the queue
    void enqueue(const QAVFrame &frame);

    // Returns size of frames in the queue
    size_t size() const;

    // Directly writes to the encoder
    int write(const QAVFrame &frame);
    int write(const QAVSubtitleFrame &frame);

private:
    int initStreams(const QList<EncoderStream> &streams, Locker &);
    void init(Locker &);
    int initStream(const EncoderStream &stream, int index, AVStream *out_stream, Locker &);
    // Need to make a copy of frame
    // streamIndex is needed to flush empty frame
    int write(QAVFrame frame, int streamIndex, Locker &);
    int write(QAVSubtitleFrame frame, int streamIndex, Locker &);
    void stop(Locker &);
    void reset(Locker &) override;
    int flushFrames(Locker &) override;

    Q_DECLARE_PRIVATE(QAVMuxerFrames)
};

QT_END_NAMESPACE

#endif
