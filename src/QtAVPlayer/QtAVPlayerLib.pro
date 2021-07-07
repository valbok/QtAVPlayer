TARGET = QtAVPlayer
MODULE = QtAVPlayer

QT = multimedia concurrent
QT_PRIVATE += gui-private

INCLUDEPATH += $$absolute_path($$PWD/..)
message ("INCLUDEPATH: " $$INCLUDEPATH)

#QMAKE_USE += ffmpeg

PRIVATE_HEADERS += \
    qavcodec_p.h \
    qavcodec_p_p.h \
    qavaudiocodec_p.h \
    qavvideocodec_p.h \
    qavhwdevice_p.h \
    qavdemuxer_p.h \
    qavpacket_p.h \
    qavframe_p.h \
    qavpacketqueue_p.h \
    qavvideobuffer_p.h \
    qavvideobuffer_cpu_p.h \
    qavvideobuffer_gpu_p.h

PUBLIC_HEADERS += \
    qavaudioformat.h \
    qavframe.h \
    qavvideoframe.h \
    qavaudioframe.h \    
    qtavplayerglobal.h \
    qavaudiooutput.h \
    qavplayer.h

SOURCES += \
    qavaudiooutput.cpp \
    qavplayer.cpp \
    qavcodec.cpp \
    qavaudiocodec.cpp \
    qavvideocodec.cpp \
    qavdemuxer.cpp \
    qavpacket.cpp \
    qavframe.cpp \
    qavvideoframe.cpp \
    qavaudioframe.cpp \
    qavvideobuffer_cpu.cpp \
    qavvideobuffer_gpu.cpp

qtConfig(va_x11):qtConfig(opengl): {
    QMAKE_USE += va_x11 x11
    PRIVATE_HEADERS += qavhwdevice_vaapi_x11_glx_p.h
    SOURCES += qavhwdevice_vaapi_x11_glx.cpp
}

qtConfig(va_drm):qtConfig(egl): {
    QMAKE_USE += va_drm egl
    PRIVATE_HEADERS += qavhwdevice_vaapi_drm_egl_p.h
    SOURCES += qavhwdevice_vaapi_drm_egl.cpp
}

macos|darwin {
    PRIVATE_HEADERS += qavhwdevice_videotoolbox_p.h
    SOURCES += qavhwdevice_videotoolbox.mm
    LIBS += -framework CoreVideo -framework Metal -framework CoreMedia -framework QuartzCore -framework IOSurface
}

win32 {
    PRIVATE_HEADERS += qavhwdevice_d3d11_p.h
    SOURCES += qavhwdevice_d3d11.cpp
}

android {
    PRIVATE_HEADERS += qavhwdevice_mediacodec_p.h
    SOURCES += qavhwdevice_mediacodec.cpp

    LIBS += -lavcodec -lavformat -lswscale -lavutil -lswresample
    equals(ANDROID_TARGET_ARCH, armeabi-v7a): \
        LIBS += -L$$(AVPLAYER_ANDROID_LIB_ARMEABI_V7A)

    equals(ANDROID_TARGET_ARCH, arm64-v8a): \
        LIBS += -L$$(AVPLAYER_ANDROID_LIB_ARMEABI_V8A)

    equals(ANDROID_TARGET_ARCH, x86): \
        LIBS += -L$$(AVPLAYER_ANDROID_LIB_X86)

    equals(ANDROID_TARGET_ARCH, x86_64): \
        LIBS += -L$$(AVPLAYER_ANDROID_LIB_X86_64)
}

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS
#load(qt_module)

TEMPLATE = lib
DEFINES += QT_BUILD_AVPLAYER_LIB
