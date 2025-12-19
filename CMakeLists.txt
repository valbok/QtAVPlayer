cmake_minimum_required(VERSION 3.20)

# Set project properties
set(PRJ_COMPANY_ID          valbok)
set(PRJ_COMPANY_NAME        "Valbok")
set(PRJ_ID                  qtavplayer)
set(PRJ_READABLE_NAME       "QtAVPlayer")
set(PRJ_VERSION_SEMANTIC    0.0.1)
set(PRJ_DESCRIPTION         "Use FFMpeg library with Qt framework.")
set(PRJ_LICENSE             "MIT")
set(PRJ_COPYRIGHT           "Copyright (c) 2020-2025 ${PRJ_COMPANY_NAME} (${PRJ_LICENSE} License)")
set(PRJ_HOMEPAGE_URL        "https://github.com/valbok/QtAVPlayer")

set(PROJECT_NAME ${PRJ_ID})
set(PROJECT_VERSION_CPP_MIN 17)

# Set project configuration
## Generic properties
project(${PROJECT_NAME}
    LANGUAGES       CXX
    VERSION         "${PRJ_VERSION_SEMANTIC}"
    DESCRIPTION     ${PRJ_DESCRIPTION}
    HOMEPAGE_URL    ${PRJ_HOMEPAGE_URL}
)

## Qt specific properties
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Define project options
# Example: option(QTAVPLAYER_BUILD_TESTS "Use to enable/disable build of unit-tests." ON)
option(QTAVPLAYER_MULTIMEDIA                "Use to enable/disable QtMultimedia support" OFF)
option(QTAVPLAYER_WIDGET_OPENGL             "Use to enable/disable OpenGL widget support" OFF)

option(QTAVPLAYER_HW_SUPPORT_WINDOWS        "Use to enable/disable hardware accelerated support for Windows" ON)
option(QTAVPLAYER_HW_SUPPORT_MACOS          "Use to enable/disable hardware accelerated support for MacOS" ON)
option(QTAVPLAYER_HW_SUPPORT_LINUX_VA_DRM   "Use to enable/disable hardware accelerated support for Linux libva-drm" OFF)
option(QTAVPLAYER_HW_SUPPORT_LINUX_VA_X11   "Use to enable/disable hardware accelerated support for Linux libva-x11" OFF)
option(QTAVPLAYER_HW_SUPPORT_LINUX_VDPAU    "Use to enable/disable hardware accelerated support for Linux libvdpau" OFF)
option(QTAVPLAYER_HW_SUPPORT_ANDROID        "Use to enable/disable hardware accelerated support for Android" ON)
option(QTAVPLAYER_HW_SUPPORT_IOS            "Use to enable/disable hardware accelerated support for iOS" ON)

# Manage global compiler options
## For MSVC: force to read source code as UTF-8 file (already default behaviour on GCC/Clang)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Set C++ standard to use
if(DEFINED CMAKE_CXX_STANDARD)
    if(${CMAKE_CXX_STANDARD} LESS ${PROJECT_VERSION_CPP_MIN})
        message(FATAL_ERROR "Project ${PROJECT_NAME} require at least C++ standard ${PROJECT_VERSION_CPP_MIN}")
    endif()
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "Project \"${PROJECT_NAME}\" compiled with C++ standard ${CMAKE_CXX_STANDARD}")

# Set needed packages
## FFmpeg (since Qt also embed his own version of FFmpeg, package must be searched BEFORE Qt)
find_package(FFMPEG 7.0.0 REQUIRED)

## Qt packages
set(PROJECT_QT_PACKAGES
    Core
    Gui
)

if(QTAVPLAYER_MULTIMEDIA)
    set(PROJECT_QT_PACKAGES ${PROJECT_QT_PACKAGES}
        Multimedia
    )
endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS ${PROJECT_QT_PACKAGES} REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${PROJECT_QT_PACKAGES} REQUIRED)

list(TRANSFORM PROJECT_QT_PACKAGES PREPEND "Qt${QT_VERSION_MAJOR}::")

## Qt private packages
if(QTAVPLAYER_MULTIMEDIA)
    if(${QT_VERSION_MAJOR} EQUAL 6)
        message(STATUS "Project \"${PROJECT_NAME}\" compiled with Qt6 toolchain")
        set(QT_DIR_PRIV_SOURCES ${Qt6Multimedia_PRIVATE_INCLUDE_DIRS})

    elseif(${QT_VERSION_MAJOR} EQUAL 5)
        message(STATUS "Project \"${PROJECT_NAME}\" compiled with Qt5 toolchain")
        set(QT_DIR_PRIV_SOURCES ${Qt5Multimedia_PRIVATE_INCLUDE_DIRS})

    else()
        message(FATAL_ERROR "Project ${PROJECT_NAME} only support Qt5 or Qt6 toolchains, found Qt${QT_VERSION_MAJOR} toolchain")
    endif()
endif()

## Other library packages
### Example: find_package(nlohmann_json 3.11.3 REQUIRED)

# Defines useful path variables for easier CMake configuration
set(PROJECT_DIR_PUBLIC_HEADERS_ROOT "${PROJECT_SOURCE_DIR}/include")
set(PROJECT_DIR_PUBLIC_HEADERS      "${PROJECT_DIR_PUBLIC_HEADERS_ROOT}/${PROJECT_NAME}")
set(PROJECT_DIR_PRIV_SOURCES        "${PROJECT_SOURCE_DIR}/src")

set(PROJECT_FILE_GLOBAL             "${PROJECT_DIR_PUBLIC_HEADERS}/qtavplayerglobal.h")

set(PROJECT_FILE_OS_METADATA_WINDOWS "${PROJECT_SOURCE_DIR}/metadata/windows/infos.rc")

# Manage library files
## Configure file project - File containing macros that can be used in project
configure_file("${PROJECT_FILE_GLOBAL}.in" "${PROJECT_FILE_GLOBAL}")
configure_file("${PROJECT_FILE_OS_METADATA_WINDOWS}.in" "${PROJECT_FILE_OS_METADATA_WINDOWS}")

## Set library compilation definitions
set(PROJECT_DEFS_PRIV
    "QTAVPLAYER_LIBRARY_BUILD"
)

set(PROJECT_DEFS_PUB

)

## Set library linking
### Linking dirs
set(PROJECT_LINKING_DIR
    ${FFMPEG_LIBRARY_DIRS}
)

### Linking libs
set(PROJECT_LINKING_LIB
    # Example: nlohmann_json::nlohmann_json
    ${PROJECT_QT_PACKAGES}
    ${FFMPEG_LIBRARIES}
)

## Set library files
### Public interface
set(PROJECT_HEADERS_PUBLIC
    qavaudioconverter.h
    qavaudioformat.h
    qavaudioframe.h
    qavframe.h
    qaviodevice.h
    qavmuxer.h
    qavpacket.h
    qavplayer.h
    qavstream.h
    qavstreamframe.h
    qavsubtitleframe.h
    qavvideoframe.h
)

### Private
set(PROJECT_HEADERS_PRIVATE
    qavaudiocodec_p.h
    qavaudiofilter_p.h
    qavaudioinputfilter_p.h
    qavaudiooutputfilter_p.h
    qavcodec_p.h
    qavcodec_p_p.h
    qavdemuxer_p.h
    qavfilter_p.h
    qavfilter_p_p.h
    qavfiltergraph_p.h
    qavfilters_p.h
    qavframe_p.h
    qavframecodec_p.h
    qavhwdevice_p.h
    qavinoutfilter_p.h
    qavinoutfilter_p_p.h
    qavpacketqueue_p.h
    qavstreamframe_p.h
    qavsubtitlecodec_p.h
    qavvideobuffer_cpu_p.h
    qavvideobuffer_gpu_p.h
    qavvideobuffer_p.h
    qavvideocodec_p.h
    qavvideofilter_p.h
    qavvideoinputfilter_p.h
    qavvideooutputfilter_p.h
)

set(PROJECT_SOURCES
    qavaudiocodec.cpp
    qavaudioconverter.cpp
    qavaudiofilter.cpp
    qavaudioframe.cpp
    qavaudioinputfilter.cpp
    qavaudiooutputfilter.cpp
    qavcodec.cpp
    qavdemuxer.cpp
    qavfilter.cpp
    qavfiltergraph.cpp
    qavfilters.cpp
    qavframe.cpp
    qavframecodec.cpp
    qavinoutfilter.cpp
    qaviodevice.cpp
    qavmuxer.cpp
    qavpacket.cpp
    qavplayer.cpp
    qavstream.cpp
    qavstreamframe.cpp
    qavsubtitlecodec.cpp
    qavsubtitleframe.cpp
    qavvideobuffer_cpu.cpp
    qavvideobuffer_gpu.cpp
    qavvideocodec.cpp
    qavvideofilter.cpp
    qavvideoframe.cpp
    qavvideoinputfilter.cpp
    qavvideooutputfilter.cpp
)

set(PROJECT_RESSOURCES
    # No ressources to set
)

# Manage specific features
if(QTAVPLAYER_MULTIMEDIA)
    message(STATUS "Project \"${PROJECT_NAME}\" compiled with multimedia support")
    set(PROJECT_HEADERS_PUBLIC ${PROJECT_HEADERS_PUBLIC}
        qavaudiooutput.h
        qavaudiooutputdevice.h
    )

    set(PROJECT_SOURCES ${PROJECT_SOURCES}
        qavaudiooutput.cpp
        qavaudiooutputdevice.cpp
    )

    set(PROJECT_DEFS_PUB ${PROJECT_DEFS_PUB}
        "QT_AVPLAYER_MULTIMEDIA"
    )
endif()

if(QTAVPLAYER_WIDGET_OPENGL)
    message(STATUS "Project \"${PROJECT_NAME}\" compiled with OpenGL widget support")
    set(PROJECT_HEADERS_PUBLIC ${PROJECT_HEADERS_PUBLIC}
        qavwidget_opengl.h
    )

    set(PROJECT_SOURCES ${PROJECT_SOURCES}
        qavwidget_opengl.cpp
    )

    set(PROJECT_DEFS_PUB ${PROJECT_DEFS_PUB}
        "QT_AVPLAYER_WIDGET_OPENGL"
    )
endif()

# Manage platform differences
## Refer to: https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html
# Manage platform differences
## Refer to: https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PROJECT_DEFS_PUB ${PROJECT_DEFS_PUB}
        "NOMINMAX"  # Needed because Windows define macros "min/max", better to not defined those and use "std::min/std::max" instead
        "WIN32_LEAN_AND_MEAN=1"
    )

    set(PROJECT_RESSOURCES ${PROJECT_RESSOURCES}
        ${PROJECT_FILE_OS_METADATA_WINDOWS}
    )

    if(QTAVPLAYER_HW_SUPPORT_WINDOWS)
        message(STATUS "Project \"${PROJECT_NAME}\" have enabled accelerated hardware support for \"Windows\" systems")

        set(PROJECT_HEADERS_PRIVATE ${PROJECT_HEADERS_PRIVATE}
            qavhwdevice_d3d11_p.h
        )

        set(PROJECT_SOURCES ${PROJECT_SOURCES}
            qavhwdevice_d3d11.cpp
        )

        set(PROJECT_DEFS_PRIV ${PROJECT_DEFS_PRIV}
            "QT_AVPLAYER_D3D11"
        )
    endif()
endif()

if((CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND QTAVPLAYER_HW_SUPPORT_MACOS) OR (CMAKE_SYSTEM_NAME STREQUAL "iOS" AND QTAVPLAYER_HW_SUPPORT_IOS))
    message(STATUS "Project \"${PROJECT_NAME}\" have enabled accelerated hardware support for \"Apple\" systems")

    set(PROJECT_SOURCES ${PROJECT_SOURCES}
        qavhwdevice_videotoolbox.mm
    )

    set(PROJECT_DEFS_PRIV ${PROJECT_DEFS_PRIV}
        "QT_AVPLAYER_VTOOLBOX"
    )

    find_library(COREVIDEO_FRAMEWORK CoreVideo)
    find_library(IOSURFACE_FRAMEWORK IOSurface)
    find_library(METAL_FRAMEWORK Metal)
    set(PROJECT_LINKING_LIB ${PROJECT_LINKING_LIB}
        ${COREVIDEO_FRAMEWORK}
        ${IOSURFACE_FRAMEWORK}
        ${METAL_FRAMEWORK}
    )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(QTAVPLAYER_HW_SUPPORT_LINUX_VA_DRM)
        message(STATUS "Project \"${PROJECT_NAME}\" have enabled accelerated hardware support for \"Linux va-drm\" systems")

        set(PROJECT_HEADERS_PRIVATE ${PROJECT_HEADERS_PRIVATE}
            qavhwdevice_vaapi_drm_egl_p.h
        )

        set(PROJECT_SOURCES ${PROJECT_SOURCES}
            qavhwdevice_vaapi_drm_egl.cpp
        )

        set(PROJECT_DEFS_PRIV ${PROJECT_DEFS_PRIV}
            "QT_AVPLAYER_VA_DRM"
        )

        set(PROJECT_LINKING_LIB ${PROJECT_LINKING_LIB}
            EGL
            va      #libva
            va-drm
        )
    endif()

    if(QTAVPLAYER_HW_SUPPORT_LINUX_VA_X11)
        message(STATUS "Project \"${PROJECT_NAME}\" have enabled accelerated hardware support for \"Linux va-x11\" systems")

        set(PROJECT_HEADERS_PRIVATE ${PROJECT_HEADERS_PRIVATE}
            qavhwdevice_vaapi_x11_glx_p.h
        )

        set(PROJECT_SOURCES ${PROJECT_SOURCES}
            qavhwdevice_vaapi_x11_glx.cpp
        )

        set(PROJECT_DEFS_PRIV ${PROJECT_DEFS_PRIV}
            "QT_AVPLAYER_VA_X11"
        )

        set(PROJECT_LINKING_LIB ${PROJECT_LINKING_LIB}
            va      #libva
            va-x11
        )
    endif()

    if(QTAVPLAYER_HW_SUPPORT_LINUX_VDPAU)
        message(STATUS "Project \"${PROJECT_NAME}\" have enabled accelerated hardware support for \"Linux vdpau\" systems")

        set(PROJECT_HEADERS_PRIVATE ${PROJECT_HEADERS_PRIVATE}
            qavhwdevice_vdpau_p.h
        )

        set(PROJECT_SOURCES ${PROJECT_SOURCES}
            qavhwdevice_vdpau.cpp
        )

        set(PROJECT_DEFS_PRIV ${PROJECT_DEFS_PRIV}
            "QT_AVPLAYER_VDPAU"
        )
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Android" AND QTAVPLAYER_HW_SUPPORT_ANDROID)
    message(STATUS "Project \"${PROJECT_NAME}\" have enabled accelerated hardware support for \"Android\" systems")

    set(PROJECT_HEADERS_PRIVATE ${PROJECT_HEADERS_PRIVATE}
        qavhwdevice_mediacodec_p.h
    )

    set(PROJECT_SOURCES ${PROJECT_SOURCES}
        qavhwdevice_mediacodec.cpp
        qavandroidsurfacetexture.cpp
    )

    set(PROJECT_DEFS_PRIV ${PROJECT_DEFS_PRIV}
        "QT_AVPLAYER_MEDIACODEC"
    )
endif()

# Manage options
## Example: if(QTAVPLAYER_USE_CUSTOM_BEHAVIOUR)

### Prepend all files to proper directory
list(TRANSFORM PROJECT_HEADERS_PUBLIC PREPEND "${PROJECT_DIR_PUBLIC_HEADERS}/")
list(TRANSFORM PROJECT_HEADERS_PRIVATE PREPEND "${PROJECT_DIR_PRIV_SOURCES}/")
list(TRANSFORM PROJECT_SOURCES PREPEND "${PROJECT_DIR_PRIV_SOURCES}/")

set(PROJECT_FILES ${PROJECT_RESSOURCES} ${PROJECT_FILE_GLOBAL} ${PROJECT_HEADERS_PUBLIC} ${PROJECT_HEADERS_PRIVATE} ${PROJECT_SOURCES})

# Add files to the library
add_library(${PROJECT_NAME} SHARED ${PROJECT_FILES})

# Set version of library
set_target_properties(${PROJECT_NAME} PROPERTIES
                        VERSION ${PROJECT_VERSION}
                        SOVERSION ${PROJECT_VERSION_MAJOR})

# Link needed libraries
target_link_directories(${PROJECT_NAME} PUBLIC ${PROJECT_LINKING_DIR})
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC ${PROJECT_LINKING_LIB})

# Specify compiler options
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Werror=return-type -Wshadow -Wlogical-op -Wduplicated-cond -Wduplicated-branches>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Werror=return-type -Wshadow -Wlogical-op -Wduplicated-cond -Wduplicated-branches>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /wd4251>
)

# Compile needed definitions
## Library related
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_DEFS_PUB})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_DEFS_PRIV})

# Specify include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${PROJECT_DIR_PUBLIC_HEADERS_ROOT}  # Only expose public headers
    PRIVATE
        ${PROJECT_DIR_PRIV_SOURCES}         # Internal headers, only for internal use
        ${FFMPEG_INCLUDE_DIRS}              # Needed for FFMPEG library
        ${QT_DIR_PRIV_SOURCES}              # Private Qt headers
)

# Do we need to build tests ?
if(QTAVPLAYER_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Add linker specific flags due to FFMPEG.
# For more details, please refer to:
# https://github.com/microsoft/vcpkg/issues/31185
if(NOT APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic")
endif()
