project(JosephOddBuild_Core
    VERSION 0.1
    DESCRIPTION "Core Lib for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBCORE_SOURCES
    job_logger.cpp
    job_timer.cpp
    job_siphash.cpp
)

set(JOBCORE_HEADERS
    crc32.h
    endian_utils.h
    real_type.h
    job_logger.h
    job_timer.h
    job_assert.h
    split_mix64.h
    aligned_allocator.h

    job_siphash.h
    job_container_concept.h
    job_list.h
    job_hash_container.h
    job_map.h
    job_obj_hash_concept.h
    job_obj_hash.h

    job_obj_concept.h
    job_base_obj.h
    job_connection.h
    job_signal.h
    job_obj_annotation.h

    job_object.h
)

add_library(JosephsOddBuilder_Core SHARED
    ${JOBCORE_HEADERS}  ${JOBCORE_SOURCES}
)

set(JOBCORE_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobcore_export.h)
generate_export_header(JosephsOddBuilder_Core
    BASE_NAME JOBCORE
    EXPORT_FILE_NAME "${JOBCORE_EXPORT}"
)

target_compile_options(JosephsOddBuilder_Core PRIVATE
    -freflection
    -fcontracts)

target_link_options(JosephsOddBuilder_Core PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_Core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobcore"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(JosephsOddBuilder_Core PUBLIC
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Simd
)

if(JOB_WINDOWS)
    ## siphash
    target_link_libraries(JosephsOddBuilder_Core PRIVATE bcrypt )
endif()

install(TARGETS JosephsOddBuilder_Core
    EXPORT JosephsOddBuilderCoreTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBCORE_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore )
install(FILES ${JOBCORE_EXPORT}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore )

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobcore.pc")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobcore.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )

    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT JosephsOddBuilderCoreTargets
    FILE JosephsOddBuilderCoreTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderCore
)
