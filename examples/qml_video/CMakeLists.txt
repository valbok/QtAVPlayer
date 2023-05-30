cmake_minimum_required(VERSION 3.8)
project(qml_video LANGUAGES CXX)

if(Qt6_FOUND)
    find_package(Qt6 COMPONENTS QmlTools Quick REQUIRED)
    qt6_add_resources(QT_RESOURCES qml_qt6.qrc)
    find_package(Qt6 COMPONENTS MultimediaQuickPrivate REQUIRED)
    include_directories(${Qt6MultimediaQuick_PRIVATE_INCLUDE_DIRS})
    include_directories(${Qt6Multimedia_PRIVATE_INCLUDE_DIRS})
else()
    find_package(Qt5 COMPONENTS Quick)
    qt5_add_resources(QT_RESOURCES qml.qrc)
    include_directories(${Qt5Multimedia_PRIVATE_INCLUDE_DIRS})
endif()

set(SOURCES main.cpp)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${QT_RESOURCES})
if(NOT WIN32)
    add_definitions(-std=c++1z)
    target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
endif()
add_dependencies(${PROJECT_NAME} QtAVPlayer)

set(LIBS QtAVPlayer ${LIBAV})

if(Qt6_FOUND)
    set(LIBS ${LIBS} Qt6::Core Qt6::Gui Qt6::Multimedia Qt6::Quick Qt6::MultimediaQuickPrivate)
else()
    set(LIBS ${LIBS} Qt5::Core Qt5::Gui Qt5::Quick Qt5::Multimedia ${MultimediaQuick_LIBRARY})
    if (Qt5Core_VERSION VERSION_LESS 5.15.0)
        find_library(MultimediaQuick_LIBRARY REQUIRED NAMES Qt5MultimediaQuick)
        set(LIBS ${LIBS} ${MultimediaQuick_LIBRARY})
    else()
        find_package(Qt5 COMPONENTS MultimediaQuick REQUIRED)
        include_directories(${Qt5MultimediaQuick_PRIVATE_INCLUDE_DIRS})
        set(LIBS ${LIBS} Qt5::MultimediaQuick)
    endif()
endif()

target_link_libraries(${PROJECT_NAME} ${LIBS})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
