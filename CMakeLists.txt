cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder
    VERSION 0.1
    LANGUAGES CXX
)

# Builkd time options
include(cmake/build_options.cmake)

## Hardware checker and some compiler flags
include(cmake/check_hardware.cmake)

## cuda crap
include(cmake/check_nvidia.cmake)

## build time deps
include(cmake/build_deps.cmake)

add_subdirectory(libs)
add_subdirectory(apps)
add_subdirectory(tests)
add_subdirectory(docs)

