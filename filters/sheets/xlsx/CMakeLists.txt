add_definitions(-DKDE_DEFAULT_DEBUG_AREA=30527)

include_directories(${KOMAIN_INCLUDES}
                    ${KOODF2_INCLUDES}           # For KoOdfChartWriter
                    ${CMAKE_SOURCE_DIR}/filters/libmso
                    ${CMAKE_SOURCE_DIR}/filters/libmsooxml
)

########### next target ###############

set(xlsx2ods_PART_SRCS
    XlsxImport.cpp
    XlsxXmlCommonReader.cpp
    XlsxXmlDocumentReader.cpp
    XlsxXmlWorksheetReader.cpp
    XlsxXmlSharedStringsReader.cpp
    XlsxXmlStylesReader.cpp
    XlsxXmlDrawingReader.cpp
    XlsxXmlChartReader.cpp
    XlsxXmlCommentsReader.cpp
    XlsxXmlTableReader.cpp

    XlsxChartOdfWriter.cpp
    FormulaParser.cpp
)

add_library(calligra_filter_xlsx2ods MODULE ${xlsx2ods_PART_SRCS})
kcoreaddons_desktop_to_json(calligra_filter_xlsx2ods calligra_filter_xlsx2ods.desktop)

target_link_libraries(
    calligra_filter_xlsx2ods
    koodf2
    komsooxml
    mso
    koodf
    komain
    calligrasheetscommon
)

install(TARGETS calligra_filter_xlsx2ods DESTINATION ${CALLIGRA_PLUGIN_INSTALL_DIR})

########### install files ###############

if(SHOULD_BUILD_FILEMANAGER_THUMBNAIL)
    install( FILES sheets_xlsx_thumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})
endif()

########## unit tests ###################

set(TestFormulaParser_SRCS
    FormulaParser.cpp
    TestFormulaParser.cpp
)

kde4_add_unit_test(TestFormulaParser TESTNAME sheets-xlsx-FormulaParser ${TestFormulaParser_SRCS})
target_link_libraries(TestFormulaParser komsooxml calligrasheetscommon Qt5::Test)
