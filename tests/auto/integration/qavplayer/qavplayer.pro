TARGET = tst_qavplayer

INCLUDEPATH += ../../../../src/ ../../../../src/QtAVPlayer
include(../../../../src/QtAVPlayer/QtAVPlayer.pri)

QT -= gui
QT += testlib
CONFIG += testcase console c++17

SOURCES += \
    tst_qavplayer.cpp
