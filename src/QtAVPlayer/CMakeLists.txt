set(SOURCES
    qavdemuxer.cpp
    qavplayer.cpp
    qavcodec.cpp
    qavframecodec.cpp
    qavaudiocodec.cpp
    qavvideocodec.cpp
    qavsubtitlecodec.cpp
    qavpacket.cpp
    qavvideobuffer_gpu.cpp
    qavvideobuffer_cpu.cpp
    qavstreamframe.cpp
    qavframe.cpp
    qavvideoframe.cpp
    qavaudioframe.cpp
    qavsubtitleframe.cpp
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
    qavstream.cpp
)
set(PUBLIC_HEADERS
    qavframe.h
    qavstreamframe.h
    qavvideoframe.h
    qavaudioframe.h
    qavsubtitleframe.h
    qavaudioformat.h
    qavplayer.h
    qavaudiooutput.h
    qtavplayerglobal.h
    qavstream.h
)

set(PRIVATE_HEADERS
    qavcodec_p.h
    qavcodec_p_p.h
    qavframecodec_p.h
    qavaudiocodec_p.h
    qavvideocodec_p.h
    qavsubtitlecodec_p.h
    qavhwdevice_p.h
    qavdemuxer_p.h
    qavpacket_p.h
    qavstreamframe_p.h
    qavframe_p.h
    qavpacketqueue_p.h
    qavvideobuffer_p.h
    qavvideobuffer_cpu_p.h
    qavvideobuffer_gpu_p.h
    qavfilter_p.h
    qavfilter_p_p.h
    qavvideofilter_p.h
    qavaudiofilter_p.h
    qavfiltergraph_p.h
    qavinoutfilter_p.h
    qavinoutfilter_p_p.h
    qavvideoinputfilter_p.h
    qavaudioinputfilter_p.h
    qavvideooutputfilter_p.h
    qavaudiooutputfilter_p.h
    qaviodevice_p.h)

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

if(vdpau)
    set(SOURCES ${SOURCES} qavhwdevice_vdpau.cpp)
endif()

if(ANDROID)
    set(SOURCES ${SOURCES} qavhwdevice_mediacodec.cpp)
endif()

if(APPLE)
    set(SOURCES ${SOURCES} qavhwdevice_videotoolbox.mm)
    find_library(IOSURFACE_LIBRARY IOSurface)
    find_library(COREVIDEO_LIBRARY CoreVideo)
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")
set_target_properties(${PROJECT_NAME} PROPERTIES PRIVATE_HEADER "${PRIVATE_HEADERS}")
if(Qt6_FOUND)
    include_directories(${Qt6Multimedia_PRIVATE_INCLUDE_DIRS})
    if(APPLE)
        target_link_libraries(${PROJECT_NAME} ${LIBS} ${LIBAV} Qt6::Core Qt6::Gui Qt6::Multimedia ${IOSURFACE_LIBRARY} ${COREVIDEO_LIBRARY})
    else()
        target_link_libraries(${PROJECT_NAME} ${LIBS} ${LIBAV} Qt6::Core Qt6::Gui Qt6::Multimedia)
    endif ()
else()
    if(APPLE)
        target_link_libraries(${PROJECT_NAME} ${LIBS} ${LIBAV} Qt5::Core Qt5::Gui Qt5::Multimedia ${IOSURFACE_LIBRARY} ${COREVIDEO_LIBRARY})
    else()
        target_link_libraries(${PROJECT_NAME} ${LIBS} ${LIBAV} Qt5::Core Qt5::Gui Qt5::Multimedia)
    endif()
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_BUILD_QTAVPLAYER_LIB)

install(TARGETS ${PROJECT_NAME}
 	LIBRARY DESTINATION lib
 	PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
    PRIVATE_HEADER DESTINATION include/${PROJECT_NAME}/private
)
