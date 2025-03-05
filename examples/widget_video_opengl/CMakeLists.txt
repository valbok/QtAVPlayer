cmake_minimum_required(VERSION 3.8)
project(widget_video_opengl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS CoreTools)
if(Qt6CoreTools_FOUND)
    find_package(Qt6 COMPONENTS MultimediaWidgets REQUIRED)
    find_package(Qt6 REQUIRED COMPONENTS Core BuildInternals OPTIONAL_COMPONENTS Multimedia OpenGL OpenGLWidgets)
    include_directories(${Qt6Multimedia_PRIVATE_INCLUDE_DIRS})
    include_directories(${Qt6MultimediaWidgets_INCLUDE_DIRS})
    include_directories(${Qt6OpenGLWidgets_INCLUDE_DIRS})
    add_definitions(${Qt6Core_DEFINITIONS})
else()
    find_package(Qt5 COMPONENTS MultimediaWidgets REQUIRED)
    find_package(Qt5 REQUIRED COMPONENTS Core OPTIONAL_COMPONENTS Gui OpenGL)
    add_definitions(${Qt5Core_DEFINITIONS})
endif()

set(QT_AVPLAYER_WIDGET_OPENGL ON CACHE BOOL "" FORCE)
set(QT_AVPLAYER_MULTIMEDIA ON CACHE BOOL "" FORCE)
include_directories(../../src/)
set(QT_AVPLAYER_DIR ../../src/QtAVPlayer/)
include(../../src/QtAVPlayer/QtAVPlayer.cmake)

set(SOURCES ${QtAVPlayer_SOURCES} main.cpp)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${QT_RESOURCES})
if(NOT WIN32)
    add_definitions(-std=c++1z)
    target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()

set(LIBS ${QtAVPlayer_LIBS})

if(Qt6_FOUND)
    set(LIBS ${LIBS} Qt6::Core Qt6::Gui Qt6::Multimedia Qt6::OpenGL Qt6::OpenGLWidgets)
else()
    set(LIBS ${LIBS} Qt5::Core Qt5::Gui Qt5::Multimedia Qt5::OpenGL)
endif()

target_link_libraries(${PROJECT_NAME} ${LIBS})
