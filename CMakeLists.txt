cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder
    VERSION 0.1
    LANGUAGES CXX
)


##############################
# Compiler flags
##############################
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW) # for reproducible timestamps
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
    message(STATUS "IPO/LTO is supported: enabling globally")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    message(WARNING "IPO/LTO is not supported: ${output}")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    add_compile_options($<$<CONFIG:Release>:-march=native>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-march=native>)
    message(STATUS "Enabling host architecture tuning (-march=native)")
endif()

add_compile_options(
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-funroll-loops>
    $<$<CONFIG:Release>:-fno-omit-frame-pointer>
)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

##############################
# Options
##############################
option(JOB_CORE_USE_DOUBLE "Use double precision" OFF)
if(JOB_CORE_USE_DOUBLE)
    add_compile_definitions(JOB_CORE_USE_DOUBLE)
endif()

option(JOB_USE_FAST_MATH "Fast Math" OFF)
if(JOB_USE_FAST_MATH)
    add_compile_options($<$<CONFIG:Release>:-ffast-math>)
endif()

option(JOB_TEST_BENCHMARKS "If def the benchhmarks" OFF)
if(JOB_TEST_BENCHMARKS)
    add_compile_options(JOB_TEST_BENCHMARKS)
endif()

# option(JOB_AMD_BUILD "AMD Build" ON)
# if(JOB_AMD_BUILD)
#     add_compile_options(JOB_AMD_BUILD)
# endif()



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

## UART
pkg_check_modules(LibUdev REQUIRED libudev) # libudev-dev

## TRANSLATIONS Fuck Translations

## DATA fun
pkg_check_modules(Flatbuffers REQUIRED flatbuffers)    # libflatbuffers-dev flatbuffers-compiler
pkg_check_modules(NlohmannJson REQUIRED nlohmann_json)   # nlohmann-json3-dev
pkg_check_modules(YAMLCpp      REQUIRED yaml-cpp)        # libyaml-cpp-dev

# sudo apt install libssl-dev libzstd-dev pkgconf libpkgconf-dev zlib1g-dev libsodium-dev libflatbuffers-dev flatbuffers-compiler nlohmann-json3-dev libyaml-cpp-dev

##############################
# sub dirs
##############################
add_subdirectory(libs)

# Later
add_subdirectory(apps)

## TESTS
enable_testing()
pkg_check_modules(CatchTwo REQUIRED catch2-with-main)   # libcatch2-dev
add_subdirectory(tests)
# CHECKPOINT: v1
