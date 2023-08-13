QT += concurrent
CONFIG += C++1z
LIBS += -lavcodec -lavformat -lswscale -lavutil -lswresample -lswscale -lavfilter -lavdevice

PRIVATE_HEADERS += \
    $$PWD/qavcodec_p.h \
    $$PWD/qavcodec_p_p.h \
    $$PWD/qavframecodec_p.h \
    $$PWD/qavaudiocodec_p.h \
    $$PWD/qavvideocodec_p.h \
    $$PWD/qavsubtitlecodec_p.h \
    $$PWD/qavhwdevice_p.h \
    $$PWD/qavdemuxer_p.h \
    $$PWD/qavpacket_p.h \
    $$PWD/qavstreamframe_p.h \
    $$PWD/qavframe_p.h \
    $$PWD/qavpacketqueue_p.h \
    $$PWD/qavvideobuffer_p.h \
    $$PWD/qavvideobuffer_cpu_p.h \
    $$PWD/qavvideobuffer_gpu_p.h \
    $$PWD/qavfilter_p.h \
    $$PWD/qavfilter_p_p.h \
    $$PWD/qavvideofilter_p.h \
    $$PWD/qavaudiofilter_p.h \
    $$PWD/qavfiltergraph_p.h \
    $$PWD/qavinoutfilter_p.h \
    $$PWD/qavinoutfilter_p_p.h \
    $$PWD/qavvideoinputfilter_p.h \
    $$PWD/qavaudioinputfilter_p.h \ 
    $$PWD/qavvideooutputfilter_p.h \
    $$PWD/qavaudiooutputfilter_p.h \
    $$PWD/qaviodevice_p.h \
    $$PWD/qavfilters_p.h

PUBLIC_HEADERS += \
    $$PWD/qavaudioformat.h \
    $$PWD/qavstreamframe.h \
    $$PWD/qavframe.h \
    $$PWD/qavvideoframe.h \
    $$PWD/qavaudioframe.h \
    $$PWD/qavsubtitleframe.h \
    $$PWD/qtavplayerglobal.h \
    $$PWD/qavstream.h \
    $$PWD/qavplayer.h \

SOURCES += \
    $$PWD/qavplayer.cpp \
    $$PWD/qavcodec.cpp \
    $$PWD/qavframecodec.cpp \
    $$PWD/qavaudiocodec.cpp \
    $$PWD/qavvideocodec.cpp \
    $$PWD/qavsubtitlecodec.cpp \
    $$PWD/qavdemuxer.cpp \
    $$PWD/qavpacket.cpp \
    $$PWD/qavframe.cpp \
    $$PWD/qavstreamframe.cpp \
    $$PWD/qavvideoframe.cpp \
    $$PWD/qavaudioframe.cpp \
    $$PWD/qavsubtitleframe.cpp \
    $$PWD/qavvideobuffer_cpu.cpp \
    $$PWD/qavvideobuffer_gpu.cpp \
    $$PWD/qavfilter.cpp \
    $$PWD/qavvideofilter.cpp \
    $$PWD/qavaudiofilter.cpp \
    $$PWD/qavfiltergraph.cpp \
    $$PWD/qavinoutfilter.cpp \
    $$PWD/qavvideoinputfilter.cpp \
    $$PWD/qavaudioinputfilter.cpp \
    $$PWD/qavvideooutputfilter.cpp \
    $$PWD/qavaudiooutputfilter.cpp \
    $$PWD/qaviodevice.cpp \
    $$PWD/qavstream.cpp \
    $$PWD/qavfilters.cpp

contains(DEFINES, QT_AVPLAYER_MULTIMEDIA) {
    QT += multimedia
    # Needed for QAbstractVideoBuffer
    equals(QT_MAJOR_VERSION, 6): QT += multimedia-private
    HEADERS += $$PWD/qavaudiooutput.h
    SOURCES += $$PWD/qavaudiooutput.cpp
}

contains(DEFINES, QT_AVPLAYER_VA_X11):qtConfig(opengl) {
    QMAKE_USE += x11 opengl
    LIBS += -lva-x11 -lva
    PRIVATE_HEADERS += $$PWD/qavhwdevice_vaapi_x11_glx_p.h
    SOURCES += $$PWD/qavhwdevice_vaapi_x11_glx.cpp
}

contains(DEFINES, QT_AVPLAYER_VA_DRM):qtConfig(egl) {
    QMAKE_USE += egl opengl
    LIBS += -lva-drm -lva
    PRIVATE_HEADERS += $$PWD/qavhwdevice_vaapi_drm_egl_p.h
    SOURCES += $$PWD/qavhwdevice_vaapi_drm_egl.cpp
}

contains(DEFINES, QT_AVPLAYER_VDPAU) {
    PRIVATE_HEADERS += $$PWD/qavhwdevice_vdpau_p.h
    SOURCES += $$PWD/qavhwdevice_vdpau.cpp
}

macos|darwin {
    PRIVATE_HEADERS += $$PWD/qavhwdevice_videotoolbox_p.h
    SOURCES += $$PWD/qavhwdevice_videotoolbox.mm
    LIBS += -framework CoreVideo -framework Metal -framework CoreMedia -framework QuartzCore -framework IOSurface
}

win32 {
    PRIVATE_HEADERS += $$PWD/qavhwdevice_d3d11_p.h
    SOURCES += $$PWD/qavhwdevice_d3d11.cpp
}

android {
    QT += core-private
    PRIVATE_HEADERS += $$PWD/qavhwdevice_mediacodec_p.h
    SOURCES += $$PWD/qavhwdevice_mediacodec.cpp $$PWD/qavandroidsurfacetexture.cpp

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
