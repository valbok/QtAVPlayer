TEMPLATE = app
TARGET = qml_video
INCLUDEPATH += . ../../src
include(../../src/QtAVPlayer/QtAVPlayer.pri)
# Example
ANDROID_EXTRA_LIBS += /opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavdevice.so \
	/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavformat.so \
	/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavutil.so \
	/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavcodec.so \
	/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libavfilter.so \
	/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswscale.so \
	/opt/mobile-ffmpeg/prebuilt/android-arm/ffmpeg/lib/libswresample.so
CONFIG += c++1z
QT += gui multimedia
lessThan(QT_MAJOR_VERSION, 6): QT += qtmultimediaquicktools-private
equals(QT_MAJOR_VERSION, 6): QT += multimediaquick-private

SOURCES += main.cpp
lessThan(QT_MAJOR_VERSION, 6): RESOURCES += qml.qrc
equals(QT_MAJOR_VERSION, 6): RESOURCES += qml_qt6.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/$$TARGET
INSTALLS += target
