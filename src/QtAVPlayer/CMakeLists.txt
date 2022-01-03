set(SOURCES
    qavdemuxer.cpp
    qavplayer.cpp
    qavcodec.cpp
    qavaudiocodec.cpp
    qavvideocodec.cpp
    qavpacket.cpp
    qavvideobuffer_gpu.cpp
    qavvideobuffer_cpu.cpp
    qavframe.cpp
    qavvideoframe.cpp
    qavaudioframe.cpp
    qavaudiooutput.cpp
    qavfilter.cpp
    qavvideofilter.cpp
    qavaudiofilter.cpp
    qavfiltergraph.cpp
    qavinoutfilter.cpp
    qavvideoinputfilter.cpp
    qavaudioinputfilter.cpp
    qavvideooutputfilter.cpp
    qavaudiooutputfilter.cpp
    qaviodevice.cpp
)
set(PUBLIC_HEADERS
    qavframe.h
    qavvideoframe.h
    qavaudioframe.h
    qavaudioformat.h
    qavplayer.h
    qavaudiooutput.h
    qtavplayerglobal.h
)

if(Qt6_FOUND)
    if(QT_FEATURE_egl AND va_drm)
        set(SOURCES ${SOURCES} qavhwdevice_vaapi_drm_egl.cpp)
        set(LIBS EGL)
    endif()
    if(QT_FEATURE_opengl AND va_x11)
        set(SOURCES ${SOURCES} qavhwdevice_vaapi_x11_glx.cpp)
    endif()
else()
    if(Qt5Gui_EGL_LIBRARIES AND va_drm)
        set(SOURCES ${SOURCES} qavhwdevice_vaapi_drm_egl.cpp)
        set(LIBS EGL)
    endif()

    if(Qt5Gui_OPENGL_LIBRARIES AND va_x11)
        set(SOURCES ${SOURCES} qavhwdevice_vaapi_x11_glx.cpp)
    endif()
endif()

if(WIN32)
    set(SOURCES ${SOURCES} qavhwdevice_d3d11.cpp)
endif()

if(ANDROID)
    set(SOURCES ${SOURCES} qavhwdevice_mediacodec.cpp)
endif()

if(MACOS)
    set(SOURCES ${SOURCES} qavhwdevice_videotoolbox.mm)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
if(Qt6_FOUND)
    target_link_libraries(${PROJECT_NAME} ${LIBS} ${LIBAV} Qt6::Core Qt6::Gui Qt6::Multimedia)
else()
    target_link_libraries(${PROJECT_NAME} ${LIBS} ${LIBAV} Qt5::Core Qt5::Gui Qt5::Multimedia)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_BUILD_QTAVPLAYER_LIB)

install(TARGETS ${PROJECT_NAME}
 	LIBRARY DESTINATION lib
 	PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
)
