TEMPLATE = app
TARGET = extract_frames
INCLUDEPATH += .

INCLUDEPATH += . ../../src/QtAVPlayer
include(../../src/QtAVPlayer/QtAVPlayer.pri)

QT -= gui
QT += QtAVPlayer

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
