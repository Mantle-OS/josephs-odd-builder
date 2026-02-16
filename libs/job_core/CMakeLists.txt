project(JosephOddBuild_Core
    VERSION 0.1
    DESCRIPTION "Core Lib for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBCORE_SOURCES
    crc32.cpp
    job_logger.cpp
    job_timer.cpp
)
set(JOBCORE_HEADERS
    crc32.h          ## polynomial crc32
    endian_utils.h   ## [from,to]LE16, [from,to]LE32 
    real_type.h      ## static and what the compiler sets as real_t
    io_base.h        ## the base of IO not net IO
    job_logger.h     ## logger clas
    job_timer.h
    job_permissions.h
    job_assert.h
    split_mix64.h
    aligned_allocator.h
)

add_library(JosephsOddBuilder_Core SHARED
    ${JOBCORE_HEADERS}
    ${JOBCORE_SOURCES}
)

set_target_properties(JosephsOddBuilder_Core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobcore"
)

target_include_directories(JosephsOddBuilder_Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/>
)

install(TARGETS JosephsOddBuilder_Core
    EXPORT JosephsOddBuilderCoreTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBCORE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobcore.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobcore.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderCoreTargets
    FILE JosephsOddBuilderCoreTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderCore
)

# CHECKPOINT: v1.0
