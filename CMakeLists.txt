cmake_minimum_required(VERSION 3.20)

set(PROJECT_NAME "qtavplayer-root")
project(${PROJECT_NAME})

option(QTAVPLAYER_BUILD_EXAMPLES    "Use to enable/disable examples build" OFF)
option(QTAVPLAYER_BUILD_TESTS       "Use to enable/disable tests build" OFF)

# Set needed features
if(QTAVPLAYER_BUILD_EXAMPLES OR QTAVPLAYER_BUILD_TESTS)
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

# Build tests
if(QTAVPLAYER_BUILD_TESTS)
    add_subdirectory(tests/auto/integration/qavplayer)
endif()
