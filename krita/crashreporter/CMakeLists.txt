include_directories(${CMAKE_SOURCE_DIR}/krita/ui)
set(crashhandler_SRCS main.cpp mainwindow.cpp)
ki18n_wrap_ui(crashhandler_SRCS wdg_crash_reporter.ui)
add_executable(crashhandler ${crashhandler_SRCS})
target_link_libraries(crashhandler kritaui Qt5::Network)
install(TARGETS crashhandler ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES KikiNurse_sm.png DESTINATION ${DATA_INSTALL_DIR}/krita/pics)
