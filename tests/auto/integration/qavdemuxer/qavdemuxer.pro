TARGET = tst_qavdemuxer

INCLUDEPATH += ../../../../src/ ../../../../src/QtAVPlayer
include(../../../../src/QtAVPlayer/QtAVPlayer.pri)

QT -= gui
QT += testlib
CONFIG += c++17 testcase console
RESOURCES += files.qrc

SOURCES += \
    tst_qavdemuxer.cpp

