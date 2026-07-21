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


## function for the massagepack generator
if(JOB_SER_MSGPACK_BINDINGS)
    include(cmake/job_msgpack_gen_lib.cmake)
endif()

if(JOB_QT_AI)
    include(cmake/cmake_ggml_build_flags.cmake)
    include(cmake/cmake_llama_cpp_build_flags.cmake)
    include(cmake/cmake_stable_diffusion_cpp_build_flags.cmake)
    add_compile_options(
            $<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>
            $<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>
    )
    add_subdirectory(3rdparty/llama.cpp)
    add_subdirectory(3rdparty/stable-diffusion.cpp)
    if (TARGET ggml)
        target_compile_options(ggml PRIVATE -fno-finite-math-only)
    endif()
    if (TARGET ggml-base)
        target_compile_options(ggml-base PRIVATE -fno-finite-math-only)
    endif()
endif()

add_subdirectory(libs)
add_subdirectory(apps)

enable_testing()
add_subdirectory(tests)

add_subdirectory(docs)

add_custom_target(github_ci SOURCES .github/workflows/test.yml )
