include_directories(${TEXTLAYOUT_INCLUDES})

add_subdirectory( tests )

########### next target ###############
set(textlayout_LIB_SRCS
    KoTextLayoutCellHelper.cpp
    KoTextLayoutArea.cpp
    KoTextLayoutArea_paint.cpp
    KoTextLayoutEndNotesArea.cpp
    KoTextLayoutTableArea.cpp
    KoTextLayoutNoteArea.cpp
    KoTextLayoutRootArea.cpp
    KoTextLayoutRootAreaProvider.cpp
    KoTextDocumentLayout.cpp
    ListItemsHelper.cpp
    KoTextShapeContainerModel.cpp
    RunAroundHelper.cpp
    KoTextLayoutObstruction.cpp
    FrameIterator.cpp
    TableIterator.cpp
    KoPointedAt.cpp
    KoTextShapeData.cpp
    FloatingAnchorStrategy.cpp
    InlineAnchorStrategy.cpp
    AnchorStrategy.cpp
    ToCGenerator.cpp
    DummyDocumentLayout.cpp
    IndexGeneratorManager.cpp
    KoStyleThumbnailer.cpp
    TextLayoutDebug.cpp
)


add_library(kotextlayout SHARED ${textlayout_LIB_SRCS})
generate_export_header(kotextlayout BASE_NAME kotextlayout)

target_link_libraries(kotextlayout kotext)
target_link_libraries(kotextlayout LINK_INTERFACE_LIBRARIES kotext)

set_target_properties(kotextlayout PROPERTIES
   VERSION ${GENERIC_CALLIGRA_LIB_VERSION} SOVERSION ${GENERIC_CALLIGRA_LIB_SOVERSION}
)
install(TARGETS kotextlayout  ${INSTALL_TARGETS_DEFAULT_ARGS})

install(
    FILES
    KoTextDocumentLayout.h
    KoTextLayoutArea.h
    KoTextLayoutRootArea.h
    KoTextShapeData.h
    KoPointedAt.h
    KoStyleThumbnailer.h
    DESTINATION
    ${INCLUDE_INSTALL_DIR}/calligra COMPONENT Devel
)
