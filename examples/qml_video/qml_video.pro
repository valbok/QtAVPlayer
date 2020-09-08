TEMPLATE = app
TARGET = qml_video
INCLUDEPATH += .

QT += gui multimedia qtmultimediaquicktools-private avplayer-private

SOURCES += main.cpp
RESOURCES += qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
