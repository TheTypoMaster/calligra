set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories(     ${CMAKE_SOURCE_DIR}/krita/sdk/tests )

macro_add_unittest_definitions()

########### next target ###############
set(kis_xcf_test_SRCS kis_xcf_test.cpp )

kde4_add_unit_test(kis_xcf_test TESTNAME krita-plugin-format-xcf_test ${kis_xcf_test_SRCS})

target_link_libraries(kis_xcf_test   kritaui Qt5::Test)
