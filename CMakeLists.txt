cmake_minimum_required(VERSION 3.8)
project(QtAVPlayer)

if(NOT WIN32)
    add_definitions(-std=c++14)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CXX_FLAGS "-Wall" "-pedantic")
    set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")
endif()

find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS Multimedia)

find_library(LIBVA_X11_LIBRARY NAMES va-x11)
find_library(LIBVA_DRM_LIBRARY NAMES va-drm)
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

option(va_x11 "Enable libva-x11" ${LIBVA_X11_FOUND})
option(va_drm "Enable libva-drm" ${LIBVA_DRM_FOUND})

configure_file(src/QtAVPlayer/qavplayer-config_p.h.in src/QtAVPlayer/qtQtAVPlayer-config_p.h)

include_directories(${Qt5Core_INCLUDE_DIRS})
include_directories(${Qt5Multimedia_INCLUDE_DIRS})
include_directories(${Qt5Gui_INCLUDE_DIRS})
include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})
include_directories(./src)

add_definitions(${Qt5Core_DEFINITIONS})

add_subdirectory(src)
add_subdirectory(examples)

