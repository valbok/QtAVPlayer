cmake_minimum_required(VERSION 3.20)

option(QTAVPLAYER_BUILD_EXAMPLES    "Use to enable/disable examples build" OFF)

# Set example needed features
if(QTAVPLAYER_BUILD_EXAMPLES)
    set(QTAVPLAYER_MULTIMEDIA ON)
    set(QTAVPLAYER_WIDGET_OPENGL ON)
endif()

# Build library
add_subdirectory(lib)

# Build examples
if(QTAVPLAYER_BUILD_EXAMPLES)
    add_subdirectory(examples/extract_frames)
    add_subdirectory(examples/qml_video)
    add_subdirectory(examples/widget_video)
    add_subdirectory(examples/widget_video_opengl)
endif()
