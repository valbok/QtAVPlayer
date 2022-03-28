TARGET = QtAVPlayer
MODULE = QtAVPlayer

QT = multimedia concurrent
equals(QT_MAJOR_VERSION, 6): QT += multimedia-private
QT_PRIVATE += gui-private

QMAKE_USE += ffmpeg

PRIVATE_HEADERS += \
    qavcodec_p.h \
    qavcodec_p_p.h \
    qavframecodec_p.h \
    qavaudiocodec_p.h \
    qavvideocodec_p.h \
    qavsubtitlecodec_p.h \
    qavhwdevice_p.h \
    qavdemuxer_p.h \
    qavpacket_p.h \
    qavstreamframe_p.h \
    qavframe_p.h \
    qavpacketqueue_p.h \
    qavvideobuffer_p.h \
    qavvideobuffer_cpu_p.h \
    qavvideobuffer_gpu_p.h \
    qavfilter_p.h \
    qavfilter_p_p.h \
    qavvideofilter_p.h \
    qavaudiofilter_p.h \
    qavfiltergraph_p.h \
    qavinoutfilter_p.h \
    qavinoutfilter_p_p.h \
    qavvideoinputfilter_p.h \
    qavaudioinputfilter_p.h \ 
    qavvideooutputfilter_p.h \
    qavaudiooutputfilter_p.h \
    qaviodevice_p.h

PUBLIC_HEADERS += \
    qavaudioformat.h \
    qavstreamframe.h \
    qavframe.h \
    qavvideoframe.h \
    qavaudioframe.h \
    qavsubtitleframe.h \
    qtavplayerglobal.h \
    qavaudiooutput.h \
    qavstream.h \
    qavplayer.h

SOURCES += \
    qavaudiooutput.cpp \
    qavplayer.cpp \
    qavcodec.cpp \
    qavframecodec.cpp \
    qavaudiocodec.cpp \
    qavvideocodec.cpp \
    qavsubtitlecodec.cpp \
    qavdemuxer.cpp \
    qavpacket.cpp \
    qavframe.cpp \
    qavstreamframe.cpp \
    qavvideoframe.cpp \
    qavaudioframe.cpp \
    qavsubtitleframe.cpp \
    qavvideobuffer_cpu.cpp \
    qavvideobuffer_gpu.cpp \
    qavfilter.cpp \
    qavvideofilter.cpp \
    qavaudiofilter.cpp \
    qavfiltergraph.cpp \
    qavinoutfilter.cpp \
    qavvideoinputfilter.cpp \
    qavaudioinputfilter.cpp \
    qavvideooutputfilter.cpp \
    qavaudiooutputfilter.cpp \
    qaviodevice.cpp \
    qavstream.cpp

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

qtConfig(vdpau) {
    PRIVATE_HEADERS += qavhwdevice_vdpau_p.h
    SOURCES += qavhwdevice_vdpau.cpp
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
load(qt_module)
