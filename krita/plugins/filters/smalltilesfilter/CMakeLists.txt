
set(kritasmalltilesfilter_PART_SRCS kis_small_tiles_filter_plugin.cpp kis_small_tiles_filter.cpp )


add_library(kritasmalltilesfilter MODULE ${kritasmalltilesfilter_PART_SRCS})
kcoreaddons_desktop_to_json(kritasmalltilesfilter kritasmalltilesfilter.desktop)


target_link_libraries(kritasmalltilesfilter kritaui)

install(TARGETS kritasmalltilesfilter  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES  kritasmalltilesfilter.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)


