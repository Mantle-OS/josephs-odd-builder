set(JOB_CUDA_TOOLKIT_ROOT_DIR    "/usr/local/cuda-13.1"              CACHE STRING "Where the cuda toolkit is ")
set(JOB_CUDA_COMPILER            "/usr/local/cuda-13.1/bin/nvcc"     CACHE STRING "full path to the nvcc compiler")
set(JOB_CUDA_HOST_COMPILER       "/usr/bin/g++-15"                   CACHE STRING "the compiler that nvcc is compiled with")
set(JOB_CUDA_ARCHITECTURES       "120"                               CACHE STRING "version of the cuda api")

if(JOB_CUDA)
    message(STATUS "Building with CUDA support")
    add_compile_definitions(JOB_CUDA_BUILD)

    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)

    find_package(CUDAToolkit REQUIRED) ## oddly this works here and is a workaround ....

    set(CMAKE_CUDA_TOOLKIT_ROOT_DIR "${JOB_CUDA_TOOLKIT_ROOT_DIR}")
    set(CMAKE_CUDA_COMPILER         "${JOB_CUDA_COMPILER}")
    set(CMAKE_CUDA_HOST_COMPILER    "${JOB_CUDA_HOST_COMPILER}")
    set(CMAKE_CUDA_ARCHITECTURES    "${JOB_CUDA_ARCHITECTURES}")
    set(CMAKE_CUDA_PROPAGATE_HOST_FLAGS OFF)
    enable_language(CUDA)
else()
    message(STATUS "CUDA Disabled: set JOB_CUDA_BUILD=ON if you want to use that. see cmake/build_options.txt")
endif()
