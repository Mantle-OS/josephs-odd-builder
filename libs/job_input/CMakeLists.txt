project(JosephOddBuild_Input
    VERSION 0.1
    DESCRIPTION "Input Event library for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBINPUT_HEADERS
    job_input_tv.h
    job_input_traits.h

    job_input_device_classification.h
    job_input_device.h
    job_input_capability.h
    job_input_dispatcher.h
    job_input_loop.h
    
)

set(JOBINPUT_SOURCES
    job_input_event.cpp
    job_input_device_classification.cpp
    job_input_dispatcher.cpp
    job_input_loop.cpp
)

set(JOBINPUT_EVENTS_HEADERS
    # parse utils
    events/job_input_device_properties.h
    events/job_input_event_types.h
    events/job_input_sync_types.h
    events/job_input_keys_events.h
    events/job_input_relative_axes.h
    events/job_input_abs_axes.h
    events/job_input_switch.h
    events/job_input_misc_events.h
    events/job_input_led_events.h
    events/job_input_auto_repeat.h
    events/job_input_sound_events.h
    events/job_input_abs_sound_events.h
    ## obj
    events/job_input_event_concept.h
    events/job_input_event.h
    events/job_input_key_event.h
    events/job_pointer_button_event.h
    events/job_pointer_motion_event.h
    events/job_pointer_scroll_event.h
    events/job_gamepad_button_event.h
    events/job_gamepad_axis_event.h
    events/job_touch_event.h
    events/job_switch_event.h
    events/job_dial_event.h
    events/job_tablet_tool_event.h
)
set(JOBINPUT_EVENTS_SOURCES)


set(JOBINPUT_LINUX_HEADERS
    linux/job_input_linux.h
    linux/job_input_linux_device.h
    linux/job_input_linux_reader.h
    linux/job_input_linux_discovery_results.h
    linux/job_input_linux_sysfs_info.h
    linux/job_input_linux_discovery.h
)

set(JOBINPUT_LINUX_SOURCES
    linux/job_input_linux_device.cpp
    linux/job_input_linux_reader.cpp
    linux/job_input_linux_discovery.cpp
)

set(JOBINPUT_LINUX_UINPUT_HEADERS
    uinput/job_input_uinput_device.cpp
    # // virtual_keyboard
    # // virtual_mouse
    # more .... we will see

)

set(JOBINPUT_LINUX_UINPUT_SOURCES
    uinput/job_input_uinput_device.h
)

set(JOBINPUT_COMPILE_THIS
    ${JOBINPUT_HEADERS}                 ${JOBINPUT_SOURCES}
    ${JOBINPUT_EVENTS_HEADERS}          ${JOBINPUT_EVENTS_SOURCES}
    ${JOBINPUT_LINUX_HEADERS}           ${JOBINPUT_LINUX_SOURCES}
    ${JOBINPUT_LINUX_UINPUT_HEADERS}    ${JOBINPUT_LINUX_UINPUT_SOURCES}
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Input SHARED ${JOBINPUT_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Input STATIC ${JOBINPUT_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Input PUBLIC JOBINPUT_STATIC_DEFINE)
endif()

set(JOB_INPUT_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/jobinput_export.h)
generate_export_header(JosephsOddBuilder_Input
    BASE_NAME JOBINPUT
    EXPORT_FILE_NAME "${JOB_INPUT_EXPORT_FILE}"
)

target_link_libraries(JosephsOddBuilder_Input
    PUBLIC
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
    PRIVATE
        Threads::Threads
        atomic
)

set_target_properties(JosephsOddBuilder_Input PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobinput"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(JosephsOddBuilder_Input PRIVATE
    -freflection
    -fcontracts
)

target_link_options(JosephsOddBuilder_Input PRIVATE
    -fcontracts
)

target_include_directories(JosephsOddBuilder_Input PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_input>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/events>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_input/events>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/linux>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_input/linux>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/uinput>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_input/uinput>

    # export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

install(TARGETS JosephsOddBuilder_Input
    EXPORT JosephsOddBuilderInputTargets
    LIBRARY     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME     DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBINPUT_HEADERS}               DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_input)
install(FILES ${JOB_INPUT_EXPORT_FILE}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_input)
install(FILES ${JOBINPUT_EVENTS_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_input/events)
install(FILES ${JOBINPUT_LINUX_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_input/linux)
install(FILES ${JOBINPUT_LINUX_UINPUT_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_input/uinput)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_input.pc")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobinput.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)
install(FILES "${PKGCONFIG_FILE}"   DESTINATION "${PKGCONFIG_INSTALL_DIR}")

install(EXPORT JosephsOddBuilderInputTargets
    FILE JosephsOddBuilderInputTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderInput
)
