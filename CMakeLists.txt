cmake_minimum_required(VERSION 3.16)
enable_testing()
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

include(GNUInstallDirs)
find_package(Threads REQUIRED)                          # Required for libcurl
find_package(PkgConfig REQUIRED)                        # pkgconf libpkgconf-dev

# pkg_check_modules(Flatbuffers REQUIRED flatbuffers)     # libflatbuffers-dev flatbuffers-compiler
pkg_check_modules(LibSodium REQUIRED libsodium)         # libsodium-dev
pkg_check_modules(NlohmannJson REQUIRED nlohmann_json)  # nlohmann-json3-dev
pkg_check_modules(CatchTwo REQUIRED catch2-with-main)   # libcatch2-dev there is also catch2-with-main.pc
pkg_check_modules(LibCurl REQUIRED libcurl)             # libcurl4-openssl-dev
pkg_check_modules(LibZstd REQUIRED libzstd)             # libzstd-dev
# pkg_check_modules(LibPistache REQUIRED libpistache)     # libpistache-dev

add_subdirectory(libs)
#add_subdirectory(apps)
add_subdirectory(tests)
