TARGET = tst_qavplayer

QT += multimedia-private testlib

include(../QtAVPlayerLib.pri)

CONFIG += testcase

SOURCES += \
    tst_qavplayer.cpp
