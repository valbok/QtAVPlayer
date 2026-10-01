TARGET = tst_qavplayer
DEFINES+="QT_AVPLAYER_MULTIMEDIA"
INCLUDEPATH += ../../../../src/ ../../../../src/QtAVPlayer
include(../../../../src/QtAVPlayer/QtAVPlayer.pri)

QT -= gui
QT += testlib
CONFIG += testcase console c++17
HEADERS += \
    tst_qavplayer.h
SOURCES += \
    tst_qavplayer.cpp \
    tst_qavplayer_audio.cpp \
    tst_qavplayer_codecs.cpp \
    tst_qavplayer_conversion.cpp \
    tst_qavplayer_core.cpp \
    tst_qavplayer_filters.cpp \
    tst_qavplayer_files.cpp \
    tst_qavplayer_io.cpp \
    tst_qavplayer_muxer.cpp \
    tst_qavplayer_seek.cpp \
    tst_qavplayer_streams.cpp \
    tst_qavplayer_video.cpp
