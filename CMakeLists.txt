cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder
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


##############################
# DEPS
##############################
include(GNUInstallDirs)
find_package(Threads REQUIRED)                          #
find_package(PkgConfig REQUIRED)                        # pkgconf libpkgconf-dev

## CRYPTO
pkg_check_modules(LibOpenSSL REQUIRED libssl)     # libssl-dev
pkg_check_modules(LibCrypto  REQUIRED libcrypto)  # libssl-dev
pkg_check_modules(LibZstd    REQUIRED libzstd)    # libzstd-dev
pkg_check_modules(LibZ       REQUIRED zlib)       # zlib1g-dev
pkg_check_modules(LibSodium  REQUIRED libsodium)  # libsodium-dev
# snappy

## TRANSLATIONS


## DATA fun
# pkg_check_modules(Flatbuffers REQUIRED flatbuffers)    # libflatbuffers-dev flatbuffers-compiler
pkg_check_modules(NlohmannJson REQUIRED nlohmann_json)   # nlohmann-json3-dev
pkg_check_modules(YAMLCpp      REQUIRED yaml-cpp)        # libyaml-cpp-dev


##############################
# sub dirs
##############################
# Core libs
add_subdirectory(libs)

# Later
#add_subdirectory(apps)

## TESTS
enable_testing()
pkg_check_modules(CatchTwo REQUIRED catch2-with-main)   # libcatch2-dev
add_subdirectory(tests)
