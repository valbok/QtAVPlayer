cmake_minimum_required(VERSION 3.10)
project(QtAVPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS CoreTools)
if(Qt6CoreTools_FOUND)
    find_package(Qt6 COMPONENTS QmlTools Quick REQUIRED)
    qt6_add_resources(QT_RESOURCES qml_qt6.qrc)
    find_package(Qt6 COMPONENTS MultimediaQuickPrivate REQUIRED)
    include_directories(${Qt6MultimediaQuick_PRIVATE_INCLUDE_DIRS})
    include_directories(${Qt6Multimedia_PRIVATE_INCLUDE_DIRS})
    find_package(Qt6 REQUIRED COMPONENTS Core BuildInternals OPTIONAL_COMPONENTS Multimedia)
    add_definitions(${Qt6Core_DEFINITIONS})
else()
    find_package(Qt5 REQUIRED COMPONENTS Core OPTIONAL_COMPONENTS Gui Multimedia)
    find_package(Qt5 COMPONENTS Quick)
    qt5_add_resources(QT_RESOURCES qml.qrc)
    include_directories(${Qt5Multimedia_PRIVATE_INCLUDE_DIRS})
    add_definitions(${Qt5Core_DEFINITIONS})
endif()

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

include_directories(..)
include(QtAVPlayer.cmake)

add_library(${PROJECT_NAME} SHARED ${QtAVPlayer_SOURCES})
set(LIBS ${QtAVPlayer_LIBS} ${LIBS})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${QtAVPlayer_PUBLIC_HEADERS}")
set_target_properties(${PROJECT_NAME} PROPERTIES PRIVATE_HEADER "${QtAVPlayer_PRIVATE_HEADERS}")

target_link_libraries(${PROJECT_NAME} ${LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_BUILD_QTAVPLAYER_LIB)

target_include_directories(${PROJECT_NAME} PUBLIC
    # Used when building this project locally
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>

    # Used by downstream users after find_package()
    # Path is relative to the installation prefix (e.g., CMAKE_INSTALL_PREFIX)
    $<INSTALL_INTERFACE:include>
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include/${PROJECT_NAME}
    PRIVATE_HEADER DESTINATION include/${PROJECT_NAME}/private
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(FILES
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  DESTINATION lib/cmake/${PROJECT_NAME})
