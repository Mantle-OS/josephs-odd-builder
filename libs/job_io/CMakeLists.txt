project(JosephOddBuild_IO
    VERSION 0.1
    DESCRIPTION "I/O abstraction layer for Joseph's Odd Builder"
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LibUdev REQUIRED libudev) # libudev-dev

set(JOBIO_SOURCES
    pty_io.cpp
    file_io.cpp
    mock_io.cpp
    io_factory.cpp

    serial/serial_info.cpp
    serial/serial_io.cpp
    serial/serial_manager.cpp
    serial/serial_settings.cpp
    serial/udev_monitor_thread.cpp

    socket/job_url.cpp
)
set(JOBIO_HEADERS
    io_base.h
    pty_io.h
    file_io.h
    mock_io.h
    io_factory.h

    serial/serial_info.h
    serial/serial_io.h
    serial/serial_manager.h
    serial/serial_settings.h
    serial/udev_monitor_thread.h

    socket/job_url.h
)

add_library(JosephsOddBuilder_IO SHARED
    ${JOBIO_HEADERS}
    ${JOBIO_SOURCES}
)

set_target_properties(JosephsOddBuilder_IO PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobio"
)

target_include_directories(JosephsOddBuilder_IO PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_include_directories(JosephsOddBuilder_IO PRIVATE
    ${LibUdev_INCLUDE_DIRS}
)

target_link_libraries(JosephsOddBuilder_IO PRIVATE
    ${LibUdev_LIBRARIES}
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Crypto
)

install(TARGETS JosephsOddBuilder_IO
    EXPORT JosephsOddBuilderIOTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBIO_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobio
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobio.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobio.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderIOTargets
    FILE JosephsOddBuilderIOTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderIO
)


