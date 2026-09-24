project(JosephOddBuild_Gui
    VERSION 0.1
    DESCRIPTION "Gui library for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBGUI_HEADERS

)
set(JOBGUI_SOURCES

)

set(JOBGUI_COLOR_HEADERS
    color/job_rgba.h # unit8_t
    color/job_rgba_16.h # unit16_t
    color/job_hsv.h
    color/job_hsl.h
    color/job_cmyk.h
    color/job_ncol.h

    color/job_color_keywords.h
    color/job_color_utils.h
    color/job_color.h
    color/job_color_codec.h
)
set(JOBGUI_COLOR_SOURCES

)

set(JOBGUI_TEXT_HEADERS

)
set(JOBGUI_TEXT_SOURCES

)

set(JOBGUI_CONTAINERS_HEADERS

)
set(JOBGUI_CONTAINERS_SOURCES

)

set(JOBGUI_LAYOUTS_HEADERS

)
set(JOBGUI_LAYOUTS_SOURCES

)

set(JOBGUI_ELEMENTS_HEADERS

)
set(JOBGUI_ELEMENTS_SOURCES

)

set(JOBGUI_COMPILE_THIS
    ${JOBGUI_HEADERS}               ${JOBGUI_SOURCES}
    ${JOBGUI_COLOR_HEADERS}         ${JOBGUI_COLOR_SOURCES}
    ${JOBGUI_TEXT_HEADERS}          ${JOBGUI_TEXT_SOURCES}
    ${JOBGUI_CONTAINERS_HEADERS}    ${JOBGUI_CONTAINERS_SOURCES}
    ${JOBGUI_LAYOUTS_HEADERS}       ${JOBGUI_LAYOUTS_SOURCES}
    ${JOBGUI_ELEMENTS_HEADERS}      ${JOBGUI_ELEMENTS_SOURCES}
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Gui SHARED ${JOBGUI_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Gui STATIC ${JOBGUI_COMPILE_THIS})
    add_compile_definitions(JOBGUI_STATIC_DEFINE)
endif()

set(JOB_GUI_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/jobgui_export.h)
generate_export_header(JosephsOddBuilder_Gui
    BASE_NAME JOBGUI
    EXPORT_FILE_NAME "${JOB_GUI_EXPORT_FILE}"
)

target_link_libraries(JosephsOddBuilder_Gui
    PUBLIC
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
        JosephsOddBuilder_Render
        ${Harfbuzz_LIBRARIES}
    PRIVATE
        Threads::Threads
        atomic
)

set_target_properties(JosephsOddBuilder_Gui PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobgui"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(JosephsOddBuilder_Gui PRIVATE
    -freflection
    -fcontracts
)

target_link_options(JosephsOddBuilder_Gui PRIVATE
    -fcontracts
)

target_include_directories(JosephsOddBuilder_Gui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_gui>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/color>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_gui/color>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/containers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_gui/containers>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/layouts>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_gui/layouts>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_gui/elements>

    # export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

install(TARGETS JosephsOddBuilder_Gui
    EXPORT JosephsOddBuilderGuiTargets
    LIBRARY     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME     DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBGUI_HEADERS}             DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_gui)
install(FILES ${JOB_GUI_EXPORT_FILE}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_gui)

install(FILES ${JOBGUI_COLOR_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_gui/color)
install(FILES ${JOBGUI_TEXT_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_gui/text)

install(FILES ${JOBGUI_ELEMENTS_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_gui/elements)
install(FILES ${JOBGUI_CONTAINERS_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_gui/containers)
install(FILES ${JOBGUI_LAYOUTS_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_gui/layouts)

# set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
# set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_gui.pc")
# configure_file(
#     "${CMAKE_CURRENT_SOURCE_DIR}/jobgui.pc.in"
#     "${PKGCONFIG_FILE}"
#     @ONLY
# )
# install(FILES "${PKGCONFIG_FILE}"   DESTINATION "${PKGCONFIG_INSTALL_DIR}")

install(EXPORT JosephsOddBuilderGuiTargets
    FILE JosephsOddBuilderGuiTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderGui
)