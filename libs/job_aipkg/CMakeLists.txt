cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder_AiPkg
    VERSION 0.1
    LANGUAGES CXX
)

cmake_policy(SET CMP0167 NEW) # crazy boost stuff
find_package(msgpack-cxx REQUIRED)
get_target_property(MSGPACK_CXX_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BOOST_LIBS Boost::boost INTERFACE_LINK_LIBRARIES)

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

add_library(JosephsOddBuilder_AiPkg SHARED
    ${JOBAIPKG_HEADERS}
    ${JOBAIPKG_SOURCES}
)

set_target_properties(JosephsOddBuilder_AiPkg PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobsdaipkg"
)

get_target_property(AIPKG_LEDGER_INCLUDES aipkg_ledger_lib INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(PKG_LIB_INCLUDES pkg_lib INTERFACE_INCLUDE_DIRECTORIES)

target_include_directories(JosephsOddBuilder_AiPkg PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobaipkg>
    $<BUILD_INTERFACE:${AIPKG_LEDGER_INCLUDES}>
    $<BUILD_INTERFACE:${PKG_LIB_INCLUDES}>
)

target_link_libraries(JosephsOddBuilder_AiPkg
    PUBLIC
        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost
        JosephsOddBuilder_Core
        ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto
        ${LibZstd_LIBRARIES} JosephsOddBuilder_Zstd
    PRIVATE
        aipkg_ledger_lib
        pkg_lib
)

install(TARGETS JosephsOddBuilder_AiPkg
    EXPORT      JosephsOddBuilderAiPkgTargets
    LIBRARY     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE     DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME     DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${JOBAIPKG_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobaipkg)

## PKGCONFIG
set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobaipkg.pc")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/jobaipkg.pc.in" "${PKGCONFIG_FILE}" @ONLY)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderAiPkgTargets
    FILE JosephsOddBuilderAiPkgTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderAiPkg
)
