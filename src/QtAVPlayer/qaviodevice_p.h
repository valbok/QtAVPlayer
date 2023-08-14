/*********************************************************
 * Copyright (C) 2021, Val Doroshchuk <valbok@gmail.com> *
 *                                                       *
 * This file is part of QtAVPlayer.                      *
 * Free Qt Media Player based on FFmpeg.                 *
 *********************************************************/

#ifndef QAVFIODEVICE_P_H
#define QAVFIODEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QIODevice>
#include <memory>

QT_BEGIN_NAMESPACE

struct AVIOContext;
class QAVIODevicePrivate;
class QAVIODevice : public QObject
{
public:
    QAVIODevice(QIODevice &device, QObject *parent = nullptr);
    ~QAVIODevice();

    AVIOContext *ctx() const;
    void abort(bool aborted);

protected:
    std::unique_ptr<QAVIODevicePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QAVIODevice)
};

QT_END_NAMESPACE

#endif
