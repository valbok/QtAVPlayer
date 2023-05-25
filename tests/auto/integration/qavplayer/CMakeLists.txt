if(MULTIMEDIA_FOUND)
    set(multimeida Qt::Multimedia)
endif()

qt_internal_add_test(tst_qavplayer
    SOURCES
        tst_qavplayer.cpp
    LIBRARIES
        ${multimeida}
        ${PROJECT_NAME}
    TESTDATA testdata
)

target_compile_definitions(tst_qavplayer PUBLIC TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata")

