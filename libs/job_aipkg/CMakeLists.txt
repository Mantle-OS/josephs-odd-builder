cmake_minimum_required(VERSION 3.16)

project(JosephsOddBuilder_AiPkg
    VERSION 0.1
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(GenerateExportHeader)

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED)
find_package(msgpack-cxx REQUIRED)

set(JOBAIPKG_HEADERS
    job_aipkg_utils.h
    job_aipkg_merkle.h
    job_aipkg_inclusion.h
    job_aipkg_consistency.h
    job_aipkg_chain.h
    job_aipkg_trust_state.h
    job_aipkg_sign.h
    job_aipkg_revocation_index.h
    job_aipkg_manifest.h
)

set(JOBAIPKG_SOURCES
    job_aipkg_utils.cpp
    job_aipkg_merkle.cpp
    job_aipkg_inclusion.cpp
    job_aipkg_consistency.cpp
    job_aipkg_chain.cpp
    job_aipkg_trust_state.cpp
    job_aipkg_sign.cpp
    job_aipkg_revocation_index.cpp
    job_aipkg_manifest.cpp
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_AiPkg SHARED
        ${JOBAIPKG_HEADERS}
        ${JOBAIPKG_SOURCES}
    )
else()
    add_library(JosephsOddBuilder_AiPkg STATIC
        ${JOBAIPKG_HEADERS}
        ${JOBAIPKG_SOURCES}
    )

    target_compile_definitions(JosephsOddBuilder_AiPkg
        PUBLIC JOBAIPKG_STATIC_DEFINE
    )
endif()

set(JOBAIPKG_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobaipkg_export.h)
generate_export_header(JosephsOddBuilder_AiPkg
    BASE_NAME JOBAIPKG
    EXPORT_FILE_NAME "${JOBAIPKG_EXPORT}"
)

set_target_properties(JosephsOddBuilder_AiPkg PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobaipkg"

    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF

    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

target_compile_options(JosephsOddBuilder_AiPkg
    PRIVATE
        -freflection
        -fcontracts
)

target_include_directories(JosephsOddBuilder_AiPkg
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobaipkg>

        ## export file
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(JosephsOddBuilder_AiPkg
    PUBLIC
        JosephsOddBuilder_Core
        JosephsOddBuilder_Crypto
        JosephsOddBuilder_Zstd

        $<BUILD_INTERFACE:aipkg_ledger_lib>
        $<BUILD_INTERFACE:pkg_lib>

        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost

    PRIVATE
        JosephsOddBuilder_IO
        ${LibSodium_LIBRARIES}
        ${LibZstd_LIBRARIES}
)

install(TARGETS JosephsOddBuilder_AiPkg
    EXPORT JosephsOddBuilderAiPkgTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaipkg
)

install(FILES ${JOBAIPKG_EXPORT}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaipkg)
install(FILES ${JOBAIPKG_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaipkg)

install(EXPORT JosephsOddBuilderAiPkgTargets
    FILE JosephsOddBuilderAiPkgTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderAiPkg
)

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobaipkg.pc")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobaipkg.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )

    install(
        FILES "${PKGCONFIG_FILE}"
        DESTINATION "${PKGCONFIG_INSTALL_DIR}"
    )
endif()