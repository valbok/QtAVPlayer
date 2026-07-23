/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#ifndef QAVSUBTITLETEXTPARSER_H
#define QAVSUBTITLEFTEXTPARSER_H

#include <QtAVPlayer/qavstream.h>
#include <QtAVPlayer/qavsubtitleframe.h>
#include <QString>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVSubtitleTextParserPrivate;
class QAVSubtitleTextParser
{
public:
    QAVSubtitleTextParser();
    ~QAVSubtitleTextParser();

    int load(const QAVStream &stream);
    void unload();

    int parseText(const QAVSubtitleFrame &frame, QString &out);

private:
    Q_DISABLE_COPY(QAVSubtitleTextParser)
    Q_DECLARE_PRIVATE(QAVSubtitleTextParser)
    std::unique_ptr<QAVSubtitleTextParserPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif
