set(kritahairypaintop_PART_SRCS
    hairy_paintop_plugin.cpp
    kis_hairy_paintop.cpp
    kis_hairy_bristle_option.cpp
    kis_hairy_shape_option.cpp
    kis_hairy_ink_option.cpp
    kis_hairy_paintop_settings.cpp
    kis_hairy_paintop_settings_widget.cpp
    bristle.cpp
    hairy_brush.cpp
    trajectory.cpp
    )

ki18n_wrap_ui(kritahairypaintop_PART_SRCS wdgInkOptions.ui  wdghairyshapeoptions.ui wdgbristleoptions.ui)

add_library(kritahairypaintop MODULE ${kritahairypaintop_PART_SRCS})
kcoreaddons_desktop_to_json(kritahairypaintop kritahairypaintop.desktop)

target_link_libraries(kritahairypaintop kritalibpaintop)

install(TARGETS kritahairypaintop  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES  kritahairypaintop.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)
install( FILES
    krita-sumi.png DESTINATION ${DATA_INSTALL_DIR}/krita/images)


