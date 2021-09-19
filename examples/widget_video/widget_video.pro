TEMPLATE = app
TARGET = widget_video
INCLUDEPATH += .

QT += gui multimedia multimediawidgets QtAVPlayer

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
