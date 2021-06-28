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

if(Qt5Gui_EGL_LIBRARIES AND VA_DRM)
    set(SOURCES ${SOURCES} qavhwdevice_vaapi_drm_egl.cpp)
    set(LIBS EGL)
endif()

if(Qt5Gui_OPENGL_LIBRARIES AND VA_X11)
    set(SOURCES ${SOURCES} qavhwdevice_vaapi_x11_glx.cpp)
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

target_link_libraries(${PROJECT_NAME} ${LIBS} ${LIBAV} Qt5::Core Qt5::Gui Qt5::Multimedia)
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_BUILD_QTAVPLAYER_LIB)

install(TARGETS ${PROJECT_NAME}
 	LIBRARY DESTINATION lib
 	PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
)
