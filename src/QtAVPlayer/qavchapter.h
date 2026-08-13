/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVCHAPTER_H
#define QAVCHAPTER_H

#include <QtAVPlayer/qtavplayerglobal.h>
#include <QMap>
#include <QString>
#include <QDebug>

QT_BEGIN_NAMESPACE

/**
 * Represents AVChapter
 */
class Q_AVPLAYER_EXPORT QAVChapter
{
public:
    QAVChapter() = default;
    QAVChapter(
        int64_t id,
        double start,
        double end,
        const QMap<QString, QString> &metadata);

    // Returns unique ID to identify the chapter
    int64_t id() const;
    
    // Returns start time in seconds
    double start() const;

    // Returns end time in seconds
    double end() const;

    // Returns parsed title from the metadata
    QString title() const;

    // Returns parsed metadata from AVChapter::metadata
    QMap<QString, QString> metadata() const;

private:
    int64_t m_id = 0;
    double m_start = 0.0;
    double m_end = 0.0;
    QMap<QString, QString> m_metadata;
};

#ifndef QT_NO_DEBUG_STREAM
Q_AVPLAYER_EXPORT QDebug operator<<(QDebug dbg, const QAVChapter &chapter);
#endif

Q_DECLARE_METATYPE(QAVChapter)

QT_END_NAMESPACE

#endif
