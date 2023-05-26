qt_internal_add_test(tst_qavplayer
    SOURCES
        tst_qavplayer.cpp
    LIBRARIES
        ${PROJECT_NAME}
    TESTDATA testdata
)

set_target_properties(Qt6::Gui PROPERTIES INTERFACE_COMPILE_DEFINITIONS "")
target_compile_definitions(tst_qavplayer PUBLIC TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata")
