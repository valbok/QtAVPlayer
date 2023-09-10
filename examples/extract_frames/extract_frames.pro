TEMPLATE = app
TARGET = extract_frames
INCLUDEPATH += .

INCLUDEPATH += . ../../src
include(../../src/QtAVPlayer/QtAVPlayer.pri)

QT -= gui

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
