

set(kritafastcolortransferfilter_PART_SRCS
    fastcolortransfer.cpp
    kis_wdg_fastcolortransfer.cpp
    )


ki18n_wrap_ui(kritafastcolortransferfilter_PART_SRCS
    wdgfastcolortransfer.ui
    )

add_library(kritafastcolortransferfilter MODULE ${kritafastcolortransferfilter_PART_SRCS})
kcoreaddons_desktop_to_json(kritafastcolortransferfilter kritafastcolortransfer.desktop)


target_link_libraries(kritafastcolortransferfilter kritaui)

install(TARGETS kritafastcolortransferfilter  DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES  kritafastcolortransfer.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/calligra)
