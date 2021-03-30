TEMPLATE = app
TARGET = extract_frames
INCLUDEPATH += .

QT += gui multimedia QtAVPlayer

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
