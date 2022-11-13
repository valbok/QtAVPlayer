/*********************************************************
 * Copyright (C) 2020, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVCODEC_P_H
#define QAVCODEC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QObject>
#include <memory>

QT_BEGIN_NAMESPACE

struct AVCodec;
struct AVCodecContext;
struct AVStream;
class QAVCodecPrivate;
class Q_AVPLAYER_EXPORT QAVCodec : public QObject
{
public:
    ~QAVCodec();

    bool open(AVStream *stream);
    AVCodecContext *avctx() const;
    void setCodec(const AVCodec *c);
    const AVCodec *codec() const;

protected:
    QAVCodec(QObject *parent = nullptr);
    QAVCodec(QAVCodecPrivate &d, QObject *parent = nullptr);
    std::unique_ptr<QAVCodecPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QAVCodec)
private:
    Q_DISABLE_COPY(QAVCodec)
};

QT_END_NAMESPACE

#endif
