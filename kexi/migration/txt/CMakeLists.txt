set(keximigrate_txtmigrate_PART_SRCS txtmigrate.cpp )


add_library(keximigrate_txt MODULE ${keximigrate_txtmigrate_PART_SRCS})



target_link_libraries(keximigrate_txt
    keximigrate
    kexiutils

    KDb
)

install(TARGETS keximigrate_txt  DESTINATION ${KEXI_PLUGIN_INSTALL_DIR})
