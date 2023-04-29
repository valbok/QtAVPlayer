TARGET = tst_qavplayer

QT += testlib QtAVPlayer QtAVPlayer-private
qtConfig(multimedia): {
    QT += multimedia-private
}

CONFIG += testcase console
CONFIG += C++1z

SOURCES += \
    tst_qavplayer.cpp
