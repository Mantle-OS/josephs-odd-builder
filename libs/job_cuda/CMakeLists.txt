project(JosephOddBuild_Cuda
    VERSION 0.1
    DESCRIPTION "Cuda Lib for Joseph's Odd Builder"
    LANGUAGES CXX CUDA
)

set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_PROPAGATE_HOST_FLAGS OFF)

# string(REPLACE "-march=native" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
message(FATAL "CXX ${COMPILE_LANGUAGE_CUDA}")


set(JOBCUDA_CU
    gemm_cuda.cu
)

add_library(JosephsOddBuilder_Cuda SHARED ${JOBCUDA_CU})

target_link_libraries(JosephsOddBuilder_Cuda PUBLIC
    CUDA::cudart
    CUDA::cublas
)
set_target_properties(JosephsOddBuilder_Cuda PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobcuda"
    # CUDA_ARCHITECTURES "89"
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    CUDA_SEPARABLE_COMPILATION ON
)


target_include_directories(JosephsOddBuilder_Cuda PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcoda/>
)

install(TARGETS JosephsOddBuilder_Cuda
    EXPORT JosephsOddBuilderCudaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBSIMD_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcuda
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobcuda.pc")

## TODO fix this as it needes to link to  cudart cublas
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobcuda.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderCudaTargets
    FILE JosephsOddBuilderCudaTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderCuda
)

