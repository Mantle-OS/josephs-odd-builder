cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder_Crypto
    VERSION 0.1
    LANGUAGES CXX
)

set(JOBCRYPTO_HEADERS
    job_crypto_utils.h
    job_crypto_init.h
    job_secure_mem.h
    job_random.h
    job_crypto_keys.h
    job_crypto_sign.h
    job_password_utils.h
    job_secret_box.h
    job_hash.h
    job_crypto.h
)

set(JOBCRYPTO_SOURCES
    job_crypto_init.cpp
    job_secure_mem.cpp
    job_random.cpp
    job_crypto_keys.cpp
    job_crypto_sign.cpp
    job_password_utils.cpp
    job_secret_box.cpp
    job_hash.cpp
    job_crypto.cpp
)

add_library(JosephsOddBuilder_Crypto SHARED
   ${JOBCRYPTO_HEADERS}
   ${JOBCRYPTO_SOURCES}
)

set(JOBCRYPTO_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobcrypto_export.h)
generate_export_header(JosephsOddBuilder_Crypto
    BASE_NAME JOBCRYPTO
    EXPORT_FILE_NAME "${JOBCRYPTO_EXPORT}"
)

set_target_properties(JosephsOddBuilder_Crypto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobcrypto"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_link_libraries(JosephsOddBuilder_Crypto PUBLIC
    Threads::Threads
    ${LibSodium_LIBRARIES}
    JosephsOddBuilder_Core
)

if(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Crypto PUBLIC
        atomic
    )
endif()

target_include_directories(JosephsOddBuilder_Crypto PUBLIC
    ${LibSodium_INCLUDE_DIRS}
)
target_include_directories(JosephsOddBuilder_Crypto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

install(TARGETS JosephsOddBuilder_Crypto
    EXPORT JosephsOddBuilderCryptoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBCRYPTO_EXPORT}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcrypto )
install(FILES ${JOBCRYPTO_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcrypto )

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobcrypto.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobcrypto.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT JosephsOddBuilderCryptoTargets
    FILE JosephsOddBuilderCryptoTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilder_Crypto
)

