TARGET = tst_qavplayer

QT -= gui
QT += testlib QtAVPlayer QtAVPlayer-private
qtConfig(multimedia): QT += multimedia-private
INCLUDEPATH += ../../../../src/QtAVPlayer
CONFIG += testcase console C++1z

SOURCES += \
    tst_qavplayer.cpp
