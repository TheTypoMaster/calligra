set(kritahatchingpaintop_PART_SRCS
    hatching_paintop_plugin.cpp
    kis_hatching_paintop.cpp
    kis_hatching_options.cpp
    kis_hatching_preferences.cpp
    kis_hatching_paintop_settings.cpp
    kis_hatching_paintop_settings_widget.cpp
    kis_hatching_pressure_crosshatching_option.cpp
    kis_hatching_pressure_separation_option.cpp
    kis_hatching_pressure_thickness_option.cpp
    hatching_brush.cpp
    )

ki18n_wrap_ui(kritahatchingpaintop_PART_SRCS wdghatchingpreferences.ui wdghatchingoptions.ui)

add_library(kritahatchingpaintop MODULE ${kritahatchingpaintop_PART_SRCS})
kcoreaddons_desktop_to_json(kritahatchingpaintop kritahatchingpaintop.desktop)

target_link_libraries(kritahatchingpaintop kritalibpaintop)

install(TARGETS kritahatchingpaintop  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES  kritahatchingpaintop.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)
install( FILES  krita-hatching.png DESTINATION ${DATA_INSTALL_DIR}/krita/images)

