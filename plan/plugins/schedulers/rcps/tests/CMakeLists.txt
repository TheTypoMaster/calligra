set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( .. ${KPLATO_INCLUDES} ${KOSTORE_INCLUDES})


########### next target ###############

set(ProjectTester_SRCS ProjectTester.cpp ../KPlatoRCPSScheduler.cpp ../KPlatoRCPSPlugin.cpp)
 
add_definitions(-DPLAN_NOPLUGIN)

kde4_add_unit_test(ProjectTester TESTNAME Plan-RCPSTester  ${ProjectTester_SRCS})
 
target_link_libraries(ProjectTester  rcps_plan planprivate kplatokernel Qt5::Test)
 

########### next target ###############
