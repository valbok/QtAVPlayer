TEMPLATE = app
TARGET = muxer
INCLUDEPATH += .

INCLUDEPATH += . ../../src
include(../../src/QtAVPlayer/QtAVPlayer.pri)

QT -= gui
CONFIG += c++1z
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
