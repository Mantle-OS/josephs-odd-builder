cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder_Zstd
    VERSION 0.1
    LANGUAGES CXX
)

set(JOBZSTD_HEADERS
    job_zstd_options.h
    job_zstd_io.h
    job_zstd_dir_guard.h
    job_zstd_entry.h
    job_zstd_wire.h
    job_zstd_compressor.h
    job_zstd_decompressor.h
    # start crypto
    job_zstd_encrypting_transport.h
    job_zstd_decrypting_transport.h
    job_zstd_compressor_crypto.h
    job_zstd_decompressor_crypto.h
    job_zstd_sign.h
    job_zstd.h
)

set(JOBZSTD_SOURCES
    job_zstd_options.cpp
    job_zstd_io.cpp
    job_zstd_wire.cpp
    job_zstd_compressor.cpp
    job_zstd_decompressor.cpp
    ## Start encryption
    job_zstd_encrypting_transport.cpp
    job_zstd_decrypting_transport.cpp
    job_zstd_compressor_crypto.cpp
    job_zstd_decompressor_crypto.cpp
    job_zstd_sign.cpp
    job_zstd.cpp
)

add_library(JosephsOddBuilder_Zstd SHARED
   ${JOBZSTD_HEADERS}
   ${JOBZSTD_SOURCES}
)

set_target_properties(JosephsOddBuilder_Zstd PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobzstd"
)

target_link_libraries(JosephsOddBuilder_Zstd PUBLIC
    Threads::Threads atomic
    JosephsOddBuilder_Core
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto
    ${LibZstd_LIBRARIES}
)

target_include_directories(JosephsOddBuilder_Zstd PRIVATE
    ${LibSodium_INCLUDE_DIRS}
)
target_include_directories(JosephsOddBuilder_Zstd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

install(TARGETS JosephsOddBuilder_Zstd
    EXPORT JosephsOddBuilderZstdTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBZSTD_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobzstd
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobzstd.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobzstd.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderZstdTargets
    FILE JosephsOddBuilderZStdTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilder_Zstd
)

