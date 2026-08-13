/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include "qavchapter.h"

QT_BEGIN_NAMESPACE

QAVChapter::QAVChapter(
    int64_t id, 
    double start,
    double end,
    const QMap<QString, QString> &metadata)
    : m_id(id)
    , m_start(start)
    , m_end(end)
    , m_metadata(metadata)
{
}

int64_t QAVChapter::id() const
{
    return m_id;
}

double QAVChapter::start() const
{
    return m_start;
}

double QAVChapter::end() const
{
    return m_end;
}

QString QAVChapter::title() const
{
    auto md = metadata();
    auto it = md.find(QLatin1String("title"));
    return it != md.end() ? *it : QString{};
}

QMap<QString, QString> QAVChapter::metadata() const
{
    return m_metadata;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QAVChapter &chapter)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QAVChapter(id=" << chapter.id()
                  << ", start=" << chapter.start()
                  << ", end=" << chapter.end()
                  << ", metadata=" << chapter.metadata()
                  << ')';
    return dbg;
}
#endif

QT_END_NAMESPACE
