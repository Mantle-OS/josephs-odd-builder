cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder
    VERSION 0.1
    LANGUAGES CXX
)

# Build time options and overrrides
include(cmake/build_options.cmake)

## Hardware checker and some compiler flags
include(cmake/check_hardware.cmake)

## cuda crap :P
include(cmake/check_nvidia.cmake)

## build time deps
include(cmake/build_deps.cmake)

## function for the massagepack generator
if(JOB_BUILD_SERIALIZER_MSGPACK)
    include(cmake/job_msgpack_gen_lib.cmake)
endif()

if(JOB_QT)
    ## Order matters here. stable diffusion cpp MUST come before Llama as stable diffusion cpp builds GGML which llama depends on
    include(cmake/cmake_stable_diffusion_cpp_build_flags.cmake)
    include(cmake/cmake_llama_cpp_build_flags.cmake)
endif()

add_subdirectory(libs)
add_subdirectory(apps)

enable_testing()
add_subdirectory(tests)

add_subdirectory(docs)

add_custom_target(github_ci SOURCES .github/workflows/test.yml )

include(cmake/cmake_print_status.cmake)