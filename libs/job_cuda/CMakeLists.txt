project(JosephOddBuild_Cuda
    VERSION 0.1
    DESCRIPTION "Cuda Lib for Joseph's Odd Builder"
    LANGUAGES CXX CUDA
)

set(JOBCUDA_CU
    gemm_cuda.cu
    cuda_bridge.cu
)

add_library(JosephsOddBuilder_Cuda SHARED ${JOBCUDA_CU})
target_compile_options(JosephsOddBuilder_Cuda PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:
        -O3
        --extended-lambda
        -lineinfo
        -Xcompiler=-march=native
        -Xcompiler=-ffast-math
        -Xcompiler=-funroll-loops
        -Xcompiler=-fno-omit-frame-pointer
        -Xcompiler=-Wall
        -Xcompiler=-Wextra
        -Xcompiler=-Werror
    >
)

set_target_properties(JosephsOddBuilder_Cuda PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobcuda"
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_DEPENDENCIES_MAP OFF
)

target_include_directories(JosephsOddBuilder_Cuda PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcoda/>
)

target_link_libraries(JosephsOddBuilder_Cuda PUBLIC
    Threads::Threads
    atomic
    ${JOB_CUDA_LIBS}
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
