project(JosephsOddBuilder_Uart
    VERSION 0.1
    DESCRIPTION "Uart abstraction layer for Joseph's Odd Builder"
    LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LibUdev REQUIRED libudev) # libudev-dev

set(JOBUART_SOURCES
    serial_info.cpp
    serial_io.cpp
    serial_manager.cpp
    serial_settings.cpp
    udev_monitor_thread.cpp
)
set(JOBUART_HEADERS
    serial_info.h
    serial_io.h
    serial_manager.h
    serial_settings.h
    udev_monitor_thread.h
)

add_library(JosephsOddBuilder_Uart SHARED
    ${JOBUART_HEADERS}
    ${JOBUART_SOURCES}
)

set_target_properties(JosephsOddBuilder_Uart PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobuart"
)

target_include_directories(JosephsOddBuilder_Uart PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_include_directories(JosephsOddBuilder_Uart PRIVATE
    ${LibUdev_INCLUDE_DIRS}
)

target_link_libraries(JosephsOddBuilder_Uart PRIVATE
    ${LibUdev_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
)

install(TARGETS JosephsOddBuilder_Uart
    EXPORT JosephsOddBuilderUartTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBUART_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobuart
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobuart.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobuart.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderUartTargets
    FILE JosephsOddBuilderUartTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderUart
)


