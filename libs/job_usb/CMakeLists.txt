project(JosephOddBuild_Usb
    VERSION 0.1
    DESCRIPTION "USB library for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBUSB_HEADERS
    job_usb_descriptor.h
    job_usb_location.h
    job_usb_strings.h
    job_usb_state.h
    job_usb_device.h

    job_usb_udev.h

    job_usb_parser.h

    job_usb_monitor.h

    job_usb.h
)

set(JOBUSB_SOURCES
    job_usb_descriptor.cpp
    job_usb_location.cpp
    job_usb_strings.cpp
    job_usb_state.cpp
    job_usb_device.cpp

    job_usb_udev.cpp

    job_usb_parser.cpp

    job_usb_monitor.cpp

    job_usb.cpp
)

set(JOBUSB_DB_HEADERS
    job_usb_id_line_kernel.h
    job_usb_id_cursor.h
    job_usb_id_grammar.h
    job_usb_id_database.h
)

set(JOBUSB_DB_SOURCES
    job_usb_id_cursor.cpp
    job_usb_id_grammar.cpp
    job_usb_id_database.cpp
)

set(JOBUSB_COMPILE_THIS
    ${JOBUSB_HEADERS}       ${JOBUSB_SOURCES}
    ${JOBUSB_DB_HEADERS}    ${JOBUSB_DB_SOURCES}
)


if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Usb SHARED ${JOBUSB_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Usb STATIC ${JOBUSB_COMPILE_THIS})
    add_compile_definitions(JOBUSB_STATIC_DEFINE)
endif()

set(JOB_USB_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/jobusb_export.h)

generate_export_header(JosephsOddBuilder_Usb
    BASE_NAME JOBUSB
    EXPORT_FILE_NAME "${JOB_USB_EXPORT_FILE}"
)

target_link_libraries(JosephsOddBuilder_Usb
    PUBLIC
      JosephsOddBuilder_Core
      JosephsOddBuilder_Threads
      JosephsOddBuilder_IO
    PRIVATE
      ${LibUsb-1.0_LIBRARIES}
      ${LibUdev_LIBRARIES}
      Threads::Threads
      atomic
)

set_target_properties(JosephsOddBuilder_Usb PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobusb"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_compile_options(JosephsOddBuilder_Usb PRIVATE
    -freflection
    -fcontracts
)

target_link_options(JosephsOddBuilder_Usb PRIVATE
    -fcontracts
)

target_include_directories(JosephsOddBuilder_Usb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_usb>
    ## exp[ort file]
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

install(TARGETS JosephsOddBuilder_Usb
    EXPORT JosephsOddBuilderUsbTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBUSB_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_usb)
install(FILES ${JOB_USB_EXPORT_FILE}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_usb)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_usb.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobusb.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}"
    DESTINATION "${PKGCONFIG_INSTALL_DIR}"
)

install(EXPORT JosephsOddBuilderUsbTargets
    FILE JosephsOddBuilderUsbTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderUsb
)
