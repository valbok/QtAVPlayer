TEMPLATE = app
TARGET = widget_video
DEFINES+="QT_AVPLAYER_MULTIMEDIA"
INCLUDEPATH += . ../../src
include(../../src/QtAVPlayer/QtAVPlayer.pri)

QT += gui multimedia multimediawidgets

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
