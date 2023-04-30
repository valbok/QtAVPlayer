cmake_minimum_required(VERSION 3.8)
project(extract_frames)

set(SOURCES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)

if(Qt6_FOUND)
    target_link_libraries(${PROJECT_NAME} QtAVPlayer Qt6::Core ${LIBAV})
else()
    target_link_libraries(${PROJECT_NAME} QtAVPlayer Qt5::Core ${LIBAV})
endif()


