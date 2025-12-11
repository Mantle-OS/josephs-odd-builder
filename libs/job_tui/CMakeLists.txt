project(JosephsOddBuilderTUI
    VERSION 0.1
    DESCRIPTION "TUI for JOB"
    LANGUAGES CXX
)

set(JOB_TUI_HEADERS
    job_tui_rect.h
    job_tui_event.h
    job_tui_draw_context.h
    job_tui_layout_engine.h
    job_tui_object.h
    job_tui_item.h
    job_tui_anchors.h
    job_tui_animation.h
    job_tui_easing.h
    job_tui_gradient.h
    job_tui_mouse_area.h
    job_tui_core_application.h
)

set(JOB_TUI_SOURCES
    job_tui_event.cpp
    job_tui_draw_context.cpp
    job_tui_object.cpp
    job_tui_item.cpp
    job_tui_mouse_area.cpp
    job_tui_core_application.cpp
    job_tui_gradient.cpp
)

set(JOB_TUI_CONTAINERS_HEADERS
    containers/window.h
    # containers/panel.h
    # containers/groupbox.h
)

set(JOB_TUI_CONTAINERS_SOURCES
    containers/window.cpp
)

set(JOB_TUI_LAYOUTS_HEADERS
    layout/linear_layout.h
    layout/grid_layout.h
    layout/row_layout.h
    layout/column_layout.h

)
set(JOB_TUI_LAYOUTS_SOURCES
    layout/linear_layout.cpp
    layout/grid_layout.cpp
    layout/column_layout.cpp
    layout/row_layout.cpp
)

set(JOB_TUI_ELEMENTS_HEADERS
    elements/rectangle.h
    elements/label.h
    # elements/toggle.h
    # elements/image.h
    # elements/textbox.h
    elements/button.h
    elements/checkbox.h
    elements/radio_button.h
)
set(JOB_TUI_ELEMENTS_SOURCES
    elements/rectangle.cpp
    elements/label.cpp
    elements/button.cpp
    elements/checkbox.cpp
    elements/radio_button.cpp
)

add_library(JosephsOddBuilder_TUI SHARED
    ${JOB_TUI_HEADERS}              ${JOB_TUI_SOURCES}
    ${JOB_TUI_CONTAINERS_HEADERS}   ${JOB_TUI_CONTAINERS_SOURCES}
    ${JOB_TUI_LAYOUTS_HEADERS}      ${JOB_TUI_LAYOUTS_SOURCES}
    ${JOB_TUI_ELEMENTS_HEADERS}     ${JOB_TUI_ELEMENTS_SOURCES}
)

target_link_libraries(JosephsOddBuilder_TUI PUBLIC
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Ansi
)

set_target_properties(JosephsOddBuilder_TUI PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobtui"
)

target_include_directories(JosephsOddBuilder_TUI PUBLIC
    # Build
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/containers>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/layouts>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/elements>
    # Install
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtui>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtui/containers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtui/layouts>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtui/elements>
)


install(TARGETS JosephsOddBuilder_TUI
    EXPORT JosephsOddBuilderTUITargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOB_TUI_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtui)
install(FILES ${JOB_TUI_CONTAINERS_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtui/containers)
install(FILES ${JOB_TUI_LAYOUTS_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtui/layouts)
install(FILES ${JOB_TUI_ELEMENTS_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtui/elements)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobtui.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobtui.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderTUITargets
    FILE JosephsOddBuilderTUITargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderTUI
)

