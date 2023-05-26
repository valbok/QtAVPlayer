qt_internal_add_test(tst_qavdemuxer
    SOURCES
        tst_qavdemuxer.cpp
    LIBRARIES
        ${PROJECT_NAME}
    TESTDATA testdata
)
set(resource_files test.wav)

qt_internal_add_resource(tst_qavdemuxer "testdata"
    PREFIX
        "/"
    BASE
        "${CMAKE_CURRENT_SOURCE_DIR}/../testdata/"
    FILES
        "../testdata/test.wav"
)

set_target_properties(Qt6::Gui PROPERTIES INTERFACE_COMPILE_DEFINITIONS "")
target_compile_definitions(tst_qavdemuxer PUBLIC TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../testdata")


