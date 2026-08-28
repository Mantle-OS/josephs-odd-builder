## GGML

## CPU
set(GGML_AVX_VNNI ON)

## BLAS
set(GGML_BLAS ON)
set(GGML_BLAS_VENDOR "Generic")

## CUDA
set(GGML_CUDA ON)

## OpenCL
set(GGML_OPENCL ON)

## Vulkan the github free runners do not have enough memory to build this.....
if(JOB_CI_BUILD)
    set(GGML_VULKAN OFF)
else()
    set(GGML_VULKAN ON)
    set(GGML_VULKAN_SHADERS_GEN_TOOLCHAIN "/usr/bin/glslc")
endif()

## Enables the RPC backend.
set(GGML_RPC ON)

## Changes the upstream OpenCL Adreno-kernel default from ON to OFF.
set(GGML_OPENCL_USE_ADRENO_KERNELS OFF)

## Upstream describes this as "CUDA graphs (llama.cpp only)".


###############################################################################
# DANGER ZONE
#
# These options differ from the known-good upstream configuration and are being
# tested individually.
#
# DO NOT uncomment these together.
#
# One wrong move and the cat gets it.
###############################################################################
## This one kills the cat. Forces cuBLAS instead of allowing GGML to select its normal MMQ/cuBLAS path.
# set(GGML_CUDA_FORCE_CUBLAS ON)


## This "works" but I want to test things .Compiles FlashAttention support for all quant types.
# set(GGML_CUDA_FA_ALL_QUANTS ON)
