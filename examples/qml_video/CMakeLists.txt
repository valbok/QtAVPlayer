cmake_minimum_required(VERSION 3.19)
project(qml_video)

find_package(Qt5 COMPONENTS Quick)

set(SOURCES main.cpp)

qt5_add_resources(QT_RESOURCES qml.qrc)

add_executable(${PROJECT_NAME} ${SOURCES} ${QT_RESOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
add_dependencies(${PROJECT_NAME} QtAVPlayer)

find_library(MultimediaQuick_LIBRARY REQUIRED NAMES Qt5MultimediaQuick)
target_link_libraries(${PROJECT_NAME} QtAVPlayer Qt5::Core Qt5::Gui Qt5::Quick Qt5::Multimedia ${MultimediaQuick_LIBRARY} ${LIBAV})

include_directories(${Qt5Multimedia_PRIVATE_INCLUDE_DIRS})


