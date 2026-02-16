project(JosephOddBuilderAnsi
    VERSION 0.1
    DESCRIPTION "Ansi for JOB"
    LANGUAGES CXX
)

set(JOB_ANSI_HEADERS
    job_ansi_line_display_mode.h
    job_ansi_attributes.h
    job_ansi_cursor.h
    job_ansi_cell.h
    job_ansi_grid.h
    job_ansi_scrollback.h
    job_ansi_saved_buffer_state.h
    job_ansi_screen.h
    job_ansi_parser.h
)

set(JOB_ANSI_SOURCES
    job_ansi_attributes.cpp
    job_ansi_cursor.cpp
    job_ansi_cell.cpp
    job_ansi_grid.cpp
    job_ansi_scrollback.cpp
    job_ansi_screen.cpp
    job_ansi_parser.cpp
)

set(JOB_ANSI_ESC_HEADERS
    esc/esc.h
    esc/osc.h
    esc/csi/csi.h
    esc/csi/mode_change.h
    esc/csi/sgr.h
    esc/csi/char_attr.h
    esc/csi/char_width.h
    esc/csi/cursor.h
    esc/csi/device_status.h
    esc/csi/dispatch_base.h
    esc/csi/rect_ops.h
    esc/csi/scroll.h
    esc/csi/tabulation.h
    esc/csi/unclassified.h
    esc/csi/window_ops.h
    esc/csi/xterm_ext.h
)
set(JOB_ANSI_ESC_SOURCES
    esc/osc.cpp
    esc/esc.cpp
    esc/csi/csi.cpp
    esc/csi/mode_change.cpp
    esc/csi/sgr.cpp
)

set(JOB_ANSI_UTILS_HEADERS
    utils/charset_translator.h
    utils/color_utils.h
    utils/calback_macros.h
    utils/rgb_color.h
    utils/utf8_decoder.h
    utils/job_ansi_utils.h
    utils/job_ansi_enums.h
    utils/job_ansi_suffix.h
)

set(JOB_ANSI_UTILS_SOURCES
    utils/utf8_decoder.cpp
    utils/job_ansi_utils.cpp
    utils/charset_translator.cpp
)

add_library(JosephsOddBuilder_Ansi SHARED
    ${JOB_ANSI_UTILS_HEADERS}   ${JOB_ANSI_UTILS_SOURCES}
    ${JOB_ANSI_ESC_HEADERS}     ${JOB_ANSI_ESC_SOURCES}
    ${JOB_ANSI_HEADERS}         ${JOB_ANSI_SOURCES}
)

target_link_libraries( JosephsOddBuilder_Ansi PRIVATE
    JosephsOddBuilder_Core
)

set_target_properties(JosephsOddBuilder_Ansi PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobansi"
)

target_include_directories(JosephsOddBuilder_Ansi PUBLIC
    # Build
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/esc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    # Install
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobansi>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobansi/esc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobansi/utils>
)

install(TARGETS JosephsOddBuilder_Ansi
    EXPORT JosephsOddBuilderAnsiTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOB_ANSI_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobansi)
install(FILES ${JOB_ANSI_ESC_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobansi/esc)
install(FILES ${JOB_ANSI_UTILS_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobansi/utils)

set(PKGCONFIG_INSTALL_DIR   "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE          "${CMAKE_CURRENT_BINARY_DIR}/jobansi.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobansi.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderAnsiTargets
    FILE JosephsOddBuilderAnsiTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderAnsi
)
