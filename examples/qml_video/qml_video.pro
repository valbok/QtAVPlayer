TEMPLATE = app
TARGET = qml_video
INCLUDEPATH += .

QT += gui multimedia QtAVPlayer
lessThan(QT_MAJOR_VERSION, 6): QT += qtmultimediaquicktools-private
equals(QT_MAJOR_VERSION, 6): QT += multimediaquick-private

SOURCES += main.cpp
lessThan(QT_MAJOR_VERSION, 6): RESOURCES += qml.qrc
equals(QT_MAJOR_VERSION, 6): RESOURCES += qml_qt6.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
