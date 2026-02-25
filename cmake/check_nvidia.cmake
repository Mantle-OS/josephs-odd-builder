set(JOB_CUDA_TOOLKIT_ROOT_DIR    "/usr/local/cuda-12.8"              CACHE STRING "Where the cuda toolkit is ")
set(JOB_CUDA_COMPILER            "/usr/local/cuda-12.8/bin/nvcc"     CACHE STRING "full path to the nvcc compiler")
set(JOB_CUDA_HOST_COMPILER       "/usr/bin/g++-12"                   CACHE STRING "the compiler that nvcc is compiled with")
set(JOB_CUDA_ARCHITECTURES       "120"                               CACHE STRING "version of the cuda api")


if(JOB_CUDA)
    message(STATUS "Building with CUDA support")

    find_package(CUDAToolkit REQUIRED)

    add_compile_definitions(JOB_CUDA_BUILD)
    set(CMAKE_CUDA_TOOLKIT_ROOT_DIR "${JOB_CUDA_TOOLKIT_ROOT_DIR}")
    set(CMAKE_CUDA_COMPILER         "${JOB_CUDA_COMPILER}")
    set(CMAKE_CUDA_HOST_COMPILER    "${JOB_CUDA_HOST_COMPILER}")
    set(CMAKE_CUDA_ARCHITECTURES    "${JOB_CUDA_ARCHITECTURES}")

    enable_language(CUDA)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        set(CMAKE_CUDA_ARCHITECTURES 120)
    endif()

    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --use_fast_math --expt-relaxed-constexpr -Xptxas=-v")

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # list(APPEND CMAKE_CUDA_FLAGS "-G")
        list(APPEND CMAKE_CUDA_FLAGS "-lineinfo")
    endif()

    message(STATUS JOSERPH ${CMAKE_CUDA_FLAGS})

else()
    message(STATUS "CUDA Disabled: set JOB_CUDA_BUILD=ON if you want to use that. see cmake/build_options.txt")
endif()
