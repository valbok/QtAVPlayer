/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVFORMATCONTEXT_P_H
#define QAVFORMATCONTEXT_P_H

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
#include <QSharedPointer>

QT_BEGIN_NAMESPACE

struct AVFormatContext;
class Q_AVPLAYER_EXPORT QAVFormatContext
{
public:
    ~QAVFormatContext();
    AVFormatContext *&ctx();
    void abort();
    bool isAborted() const;

    static QSharedPointer<QAVFormatContext> alloc(const QString &filename = {});

private:
    QAVFormatContext() = default;
    AVFormatContext *m_ctx = nullptr;
    std::atomic_bool m_abort = false;
};

QT_END_NAMESPACE

#endif
