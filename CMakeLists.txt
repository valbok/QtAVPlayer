cmake_minimum_required(VERSION 3.8)
project(QtAVPlayer)

if(NOT WIN32)
    add_definitions(-std=c++14)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CXX_FLAGS "-Wall" "-pedantic")
    set(CMAKE_CXX_FLAGS, "${CXX_FLAGS}")
endif()

set(QT_DEFAULT_MAJOR_VERSION 6)
find_package(Qt6 COMPONENTS CoreTools)
if(Qt6CoreTools_FOUND)
    # TODO: tools?
    find_package(Qt6 COMPONENTS Core GuiTools DBusTools Gui Multimedia REQUIRED)
    add_definitions(${Qt6Core_DEFINITIONS})
else()
    find_package(Qt5 COMPONENTS Core Gui Multimedia REQUIRED)
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

configure_file(src/QtAVPlayer/qavplayer-config_p.h.in src/QtAVPlayer/qtQtAVPlayer-config_p.h)

include_directories(./src)

add_subdirectory(src)
add_subdirectory(examples)

