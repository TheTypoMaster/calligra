
add_subdirectory( tests )

include_directories(${KOODF_INCLUDES}
                    ${CMAKE_SOURCE_DIR}/libs/widgetutils
                   ${KUNDO2_INCLUDES})


########### KPlato kernel library ###############

set(kplatokernel_LIB_SRCS
    kptglobal.cpp
    kpteffortcostmap.cpp
    kptdocuments.cpp
    kptaccount.cpp
    kptappointment.cpp
    kptnode.cpp
    kptproject.cpp
    kptrelation.cpp
    kptresource.cpp
    kpttask.cpp
    kptduration.cpp
    kptdatetime.cpp
    kptcalendar.cpp
    kptschedule.cpp
    kptwbsdefinition.cpp
    kptcommand.cpp
    kptpackage.cpp
    kptdebug.cpp

    kptschedulerplugin.cpp

    kptconfigbase.cpp

    KPlatoXmlLoaderBase.cpp
)

add_library(kplatokernel SHARED ${kplatokernel_LIB_SRCS})
generate_export_header(kplatokernel)

target_link_libraries(kplatokernel
PUBLIC
    koodf
    kowidgetutils
    kundo2
    KF5::KIOWidgets
    KF5::KDELibs4Support
)

set_target_properties(kplatokernel PROPERTIES VERSION ${GENERIC_CALLIGRA_LIB_VERSION} SOVERSION ${GENERIC_CALLIGRA_LIB_SOVERSION} )

install(TARGETS kplatokernel ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES plan_schedulerplugin.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR} )
