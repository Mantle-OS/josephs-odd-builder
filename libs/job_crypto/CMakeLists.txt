cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder_Crypto
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW) # for reproducible timestamps
endif()
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(LibSodium REQUIRED libsodium)

set(JOBCRYPTO_HEADERS
    job_crypto_init.h
    job_secure_mem.h
)

set(JOBCRYPTO_SOURCES
    job_crypto_init.cpp
    job_secure_mem.cpp
)

add_library(JosephsOddBuilder_Crypto SHARED
   ${JOBCRYPTO_HEADERS}
   ${JOBCRYPTO_SOURCES}
)

set_target_properties(JosephsOddBuilder_Crypto PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobcrypto"
)

target_link_libraries(JosephsOddBuilder_Crypto PUBLIC
    Threads::Threads
    ${LibSodium_LIBRARIES}
)

target_include_directories(JosephsOddBuilder_Crypto PRIVATE
    ${LibSodium_INCLUDE_DIRS}
)
target_include_directories(JosephsOddBuilder_Crypto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

install(TARGETS JosephsOddBuilder_Crypto
    EXPORT JosephsOddBuilderCryptoTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBCRYPTO_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcrypto
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobcrypto.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobcrypto.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderCryptoTargets
    FILE JosephsOddBuilderCryptoTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilder_Crypto
)

