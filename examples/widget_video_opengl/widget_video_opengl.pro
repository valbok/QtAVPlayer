TEMPLATE = app
TARGET = widget_video_opengl
QT += gui gui-private multimedia
equals(QT_MAJOR_VERSION, 6): QT += openglwidgets
DEFINES += "QT_AVPLAYER_MULTIMEDIA"
DEFINES += "QT_NO_CAST_FROM_ASCII"
DEFINES += "QT_AVPLAYER_WIDGET_OPENGL"
INCLUDEPATH += . ../../src
include(../../src/QtAVPlayer/QtAVPlayer.pri)

CONFIG += c++1z console
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
