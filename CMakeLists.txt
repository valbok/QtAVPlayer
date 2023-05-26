cmake_minimum_required(VERSION 3.8)
include(.cmake.conf)
project(QtAVPlayer
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt AVPlayer Libraries"
    HOMEPAGE_URL "http://valbok.com/"
    LANGUAGES CXX C
)

add_definitions(-std=c++17)

if(NOT WIN32)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CXX_FLAGS "-Wall" "-pedantic")
    set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")
endif()

set(QT_DEFAULT_MAJOR_VERSION 6)
set(MULTIMEDIA_FOUND FALSE)
find_package(Qt6 COMPONENTS CoreTools)
if(Qt6CoreTools_FOUND)
    # TODO: tools?
    find_package(Qt6 REQUIRED COMPONENTS Core BuildInternals OPTIONAL_COMPONENTS Multimedia Test)
    if(Qt6Multimedia_FOUND)
        set(MULTIMEDIA_FOUND TRUE)
    endif()
    add_definitions(${Qt6Core_DEFINITIONS})
else()
    find_package(Qt5 REQUIRED COMPONENTS Core OPTIONAL_COMPONENTS Gui Multimedia)
    if(Qt5Multimedia_FOUND)
        set(MULTIMEDIA_FOUND TRUE)
    endif()
    if(Qt5Gui_EGL_LIBRARIES)
        set(EGL_FOUND TRUE)
    endif()
    add_definitions(${Qt5Core_DEFINITIONS})
endif()

find_library(LIBVA_X11_LIBRARY NAMES va-x11)
find_library(LIBVA_DRM_LIBRARY NAMES va-drm)
find_library(VDPAU_LIBRARY NAMES vdpau)
find_library(AVDEVICE_LIBRARY REQUIRED NAMES avdevice)
find_library(AVCODEC_LIBRARY REQUIRED NAMES avcodec)
find_library(AVFILTER_LIBRARY REQUIRED NAMES avfilter)
find_library(AVFORMAT_LIBRARY REQUIRED NAMES avformat)
find_library(AVUTIL_LIBRARY REQUIRED NAMES avutil)
find_library(SWRESAMPLE_LIBRARY REQUIRED NAMES swresample)
find_library(SWSCALE_LIBRARY REQUIRED NAMES swscale)

set(LIBAV ${AVDEVICE_LIBRARY} ${AVFILTER_LIBRARY} ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY} ${AVUTIL_LIBRARY} ${SWRESAMPLE_LIBRARY} ${SWSCALE_LIBRARY})

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(LIBVA_X11_LIBRARY)
    set(LIBVA_X11_FOUND TRUE)
endif() 
if(LIBVA_DRM_LIBRARY)
    set(LIBVA_DRM_FOUND TRUE)
endif()
if(VDPAU_LIBRARY)
    set(VDPAU_FOUND TRUE)
endif()

option(va_x11 "Enable libva-x11" ${LIBVA_X11_FOUND})
option(va_drm "Enable libva-drm" ${LIBVA_DRM_FOUND})
option(vdpau "Enable vdpau" ${VDPAU_FOUND})
option(egl "Enable egl" ${EGL_FOUND})
option(multimedia "Enable Qt Multimedia" ${MULTIMEDIA_FOUND})
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)

configure_file(src/QtAVPlayer/qavplayer-config_p.h.in ${PROJECT_SOURCE_DIR}/src/QtAVPlayer/qtQtAVPlayer-config_p.h)
configure_file(src/QtAVPlayer/qavplayer-config.h.in ${PROJECT_SOURCE_DIR}/src/QtAVPlayer/qtQtAVPlayer-config.h)

include_directories(./src)
include_directories(./src/QtAVPlayer)
if(Qt6CoreTools_FOUND)
    qt_build_repo()
    if(Qt6Test_FOUND AND BUILD_TESTS)
        add_subdirectory(tests)
    endif()
else()
    add_subdirectory(src)
endif()

if(multimedia AND BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
