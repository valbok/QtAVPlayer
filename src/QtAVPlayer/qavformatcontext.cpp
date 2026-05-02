/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavformatcontext_p.h"
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
}

QT_BEGIN_NAMESPACE

QAVFormatContext::~QAVFormatContext()
{
    if (m_ctx) {
        avformat_close_input(&m_ctx);
        avformat_free_context(m_ctx);
    }
}

AVFormatContext *&QAVFormatContext::ctx()
{
    return m_ctx;
}

QSharedPointer<QAVFormatContext> QAVFormatContext::alloc(const QString &filename)
{
    QSharedPointer<QAVFormatContext> ret(new QAVFormatContext);
    if (filename.isEmpty()) {
        ret->m_ctx = avformat_alloc_context();
    } else {
        int err = avformat_alloc_output_context2(&ret->m_ctx, nullptr, nullptr, filename.toUtf8().constData());
        if (err) {
            qWarning() << filename << ": Could not create output context:" << err;
            return {};
        }
    }
    return ret;
}

QT_END_NAMESPACE
