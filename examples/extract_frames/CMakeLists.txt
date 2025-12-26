cmake_minimum_required(VERSION 3.20)

# Set project properties
set(PROJECT_NAME "qtavplayer-extract-frames")
set(PROJECT_VERSION_CPP_MIN 17)

# Set project configuration
## Generic properties
set(CMAKE_INCLUDE_CURRENT_DIR ON)
project(${PROJECT_NAME}
    LANGUAGES       CXX
)

## Qt specific properties
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Set C++ standard to use
if(DEFINED CMAKE_CXX_STANDARD)
    if(${CMAKE_CXX_STANDARD} LESS ${PROJECT_VERSION_CPP_MIN})
        message(FATAL_ERROR "Project ${PROJECT_NAME} require at least C++ standard ${PROJECT_VERSION_CPP_MIN}")
    endif()
endif()
message(STATUS "Project \"${PROJECT_NAME}\" compiled with C++ standard ${CMAKE_CXX_STANDARD}")

# Set needed packages
## Qt packages
find_package(QT NAMES Qt6 Qt5)  # Load Qt CMake specific cfg (allow to have access to ${QT_VERSION_MAJOR} for example)
set(PROJECT_QT_PACKAGES
    Core
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS ${PROJECT_QT_PACKAGES} REQUIRED)
list(TRANSFORM PROJECT_QT_PACKAGES PREPEND "Qt${QT_VERSION_MAJOR}::")

## Other library packages
### Example: find_package(nlohmann_json 3.11.3 REQUIRED)

# Manage application files
## Set application compilation definitions
set(PROJECT_DEFS_PRIV

)

set(PROJECT_DEFS_PUB

)

## Set library linking
### Linking dirs
set(PROJECT_LINKING_DIR

)

### Linking libs
set(PROJECT_LINKING_LIB
    ${PROJECT_QT_PACKAGES}
    qtavplayer
)

## Set application files
set(PROJECT_HEADERS

)

set(PROJECT_SOURCES
    main.cpp
)

set(PROJECT_UI_WIDGETS

)

set(PROJECT_RESSOURCES

)

# Manage platform differences
## Refer to: https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_NAME.html
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PROJECT_DEFS_PUB ${PROJECT_DEFS_PUB}
        "NOMINMAX"  # Needed because Windows define macros "min/max", better to not defined those and use "std::min/std::max" instead
        "WIN32_LEAN_AND_MEAN=1"
    )
endif()

# Add files to the application
set(PROJECT_FILES ${PROJECT_FILE_GLOBAL} ${PROJECT_HEADERS} ${PROJECT_SOURCES} ${PROJECT_UI_WIDGETS} ${PROJECT_RESSOURCES})
add_executable(${PROJECT_NAME} ${PROJECT_FILES})

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
## Application related
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_DEFS_PUB})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_DEFS_PRIV})

# Set properties of application
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE               TRUE
    WIN32_EXECUTABLE            TRUE
)