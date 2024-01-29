TEMPLATE = app
TARGET = widget_video
DEFINES += "QT_AVPLAYER_MULTIMEDIA"
DEFINES += "QT_NO_CAST_FROM_ASCII"
INCLUDEPATH += . ../../src
include(../../src/QtAVPlayer/QtAVPlayer.pri)

CONFIG += c++1z
QT += gui multimedia multimediawidgets

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
