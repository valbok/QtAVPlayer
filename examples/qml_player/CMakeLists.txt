cmake_minimum_required(VERSION 3.16)
project(qml_video_player LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Quick QuickControls2 Multimedia MultimediaQuickPrivate
)
include_directories(${Qt6MultimediaQuick_PRIVATE_INCLUDE_DIRS})
include_directories(${Qt6Multimedia_PRIVATE_INCLUDE_DIRS})

set(QTAVPLAYER_DIR "${CMAKE_SOURCE_DIR}/../../" CACHE PATH "Path to QtAVPlayer source root")

include_directories(${QTAVPLAYER_DIR})
include_directories("${QTAVPLAYER_DIR}/src")
set(QT_AVPLAYER_DIR "${QTAVPLAYER_DIR}/src/QtAVPlayer")
include("${QTAVPLAYER_DIR}/src/QtAVPlayer/QtAVPlayer.cmake")

qt_add_executable(qml_video_player
    main.cpp
    playercontroller.cpp
    playercontroller.h
    qml.qrc
    ${QtAVPlayer_SOURCES}
)

# Enable QtMultimedia integration (QVideoFrame support)
target_compile_definitions(qml_video_player PRIVATE QT_AVPLAYER_MULTIMEDIA)

target_link_libraries(qml_video_player PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::MultimediaQuickPrivate
    ${QtAVPlayer_LIBS}
)

set_target_properties(qml_video_player PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)
