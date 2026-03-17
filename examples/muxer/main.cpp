/***************************************************************
 * Copyright (C) 2020, 2026, Val Doroshchuk <valbok@gmail.com> *
 *                                                             *
 * This file is part of QtAVPlayer.                            *
 * Free Qt Media Player based on FFmpeg.                       *
 ***************************************************************/

#include <QtAVPlayer/qavplayer.h>
#include <QtAVPlayer/qavvideoframe.h>
#include <QtAVPlayer/qavaudioframe.h>
#include <QtAVPlayer/qavmuxer.h>

#include <QCoreApplication>
#include <QDebug>

#include <vector>
#include <csignal>

static int showHelp(const char *m)
{
    qDebug() << "The muxer saves content of multiple sources to one file:";
    qDebug() << m << "-f v4l2 -i /dev/video0 -f pulse -i default output.mkv";
    return 1;
}

void sigHandler(int s)
{
    std::signal(s, SIG_DFL);
    qApp->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);
    if (argc < 2) {
        return showHelp(argv[0]);
    }
    std::vector<std::pair<QString, QString>> inputSources;
    std::vector<std::unique_ptr<QAVPlayer>> players;
    QString output = QLatin1String("output.mkv");
    QString format;
    for (int i = 1; i < argc; ++i) {
        QLatin1String p(argv[i]);
        if (p == QLatin1String("-f")) {
            if (++i >= argc)
                return showHelp(argv[0]);
            format = argv[i];
        } else if (p == QLatin1String("-i")) {
            if (++i >= argc)
                return showHelp(argv[0]);
            inputSources.push_back(std::make_pair(argv[i], format));
            format = QLatin1String();
        } else if (i == argc - 1) {
            output = argv[i];
        }
    }
    if (inputSources.empty() || !format.isEmpty())
        return showHelp(argv[0]);

    QAVMuxerFrames m;
    auto load = [&](QAVPlayer::MediaStatus status) {
        static size_t playersInited = 0;
        if (status == QAVPlayer::LoadedMedia && ++playersInited == players.size()) {
            QList<QAVStream> streams;
            for (auto &p: players) {
                streams += p->availableVideoStreams() + p->availableAudioStreams() + p->availableSubtitleStreams();
            }
            auto ret = m.load(streams, output);
            if (ret < 0) {
                qDebug() << "Could not load:" << ret;
                exit(1);
            }
            for (auto &p: players)
                p->play();
            qDebug() <<"Loaded" << playersInited << "players, press CTRL+C to exit";
        }
    };

    for (size_t i = 0; i < inputSources.size(); ++i) {
        auto p = std::make_unique<QAVPlayer>();
        p->setSynced(false);
        p->setSource(inputSources[i].first);
        p->setInputFormat(inputSources[i].second);
        QObject::connect(p.get(), &QAVPlayer::videoFrame, p.get(), [&](const QAVVideoFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
        QObject::connect(p.get(), &QAVPlayer::audioFrame, p.get(), [&](const QAVAudioFrame &f) { m.enqueue(f); }, Qt::DirectConnection);
        QObject::connect(p.get(), &QAVPlayer::mediaStatusChanged, load);
        players.push_back(std::move(p));
    }

    return app.exec();
}

