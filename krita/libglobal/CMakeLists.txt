include(CheckFunctionExists)
check_function_exists(backtrace HAVE_BACKTRACE)
configure_file(config-debug.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-debug.h)

include_directories(${KOWIDGETS_INCLUDES})

set(kritaglobal_LIB_SRCS
  kis_assert.cpp
  kis_debug.cpp
  kis_icon_utils.cpp
)

kde4_add_library(kritaglobal SHARED ${kritaglobal_LIB_SRCS} )
generate_export_header(kritaglobal BASE_NAME kritaglobal)

target_link_libraries(kritaglobal kowidgets KF5::IconThemes Qt5::Concurrent Qt5::Core Qt5::Gui)
target_link_libraries(kritaglobal LINK_INTERFACE_LIBRARIES kowidgets KF5::IconThemes Qt5::Concurrent Qt5::Concurrent Qt5::Core Qt5::Gui)

set_target_properties(kritaglobal PROPERTIES
    VERSION ${GENERIC_CALLIGRA_LIB_VERSION} SOVERSION ${GENERIC_CALLIGRA_LIB_SOVERSION}
)

install(TARGETS kritaglobal  ${INSTALL_TARGETS_DEFAULT_ARGS})

