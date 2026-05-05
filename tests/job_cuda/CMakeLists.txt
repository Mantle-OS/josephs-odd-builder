cmake_minimum_required(VERSION 3.16)

project(test_job_cuda
    VERSION 0.1
    DESCRIPTION "Cuda Tests for Joseph's Odd Builder"
    LANGUAGES CXX CUDA
)

add_executable(test_job_cuda
    main.cpp
    test_sgemm.cpp
    test_gpu_bridge.cpp
)

target_link_libraries(test_job_cuda PRIVATE
    ${CatchTwo_LIBRARIES}
    CUDA::cudart
    CUDA::cublas
    CUDA::cusparse
    CUDA::curand
    JosephsOddBuilder_Cuda
    JosephsOddBuilder_Core
)

target_include_directories(test_job_cuda PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
    ${CUDAToolkit_INCLUDE_DIRECTORIES}
)

add_test(NAME job_cuda COMMAND test_job_cuda)
