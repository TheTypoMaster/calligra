include_directories( ${CMAKE_SOURCE_DIR}/karbon ${CMAKE_SOURCE_DIR}/karbon/core   )

set(karbon_refinepath_PART_SRCS RefinePathPlugin.cpp KarbonPathRefineCommand.cpp )

add_library(karbon_refinepath MODULE ${karbon_refinepath_PART_SRCS})

kcoreaddons_desktop_to_json(karbon_refinepath karbon_refinepath.desktop)

target_link_libraries(karbon_refinepath karboncommon)

install(TARGETS karbon_refinepath  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES RefinePathPlugin.rc  DESTINATION ${KDE_INSTALL_DATADIR}/karbon/plugins)
