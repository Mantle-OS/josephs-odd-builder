## GGML

# ==============================================================================
# GGML CPU Architecture & SIMD Configuration
# ==============================================================================

# Default sensible baselines
set(GGML_SSE42       ON  CACHE BOOL "ggml: enable SSE 4.2" FORCE)
set(GGML_BMI2        ON  CACHE BOOL "ggml: enable BMI2" FORCE)
set(GGML_CPU_REPACK  ON  CACHE BOOL "ggml: use runtime weight conversion of Q4_0 to Q4_X_X" FORCE)

# Initialize vector flags to OFF before cascading
set(GGML_AVX          OFF CACHE BOOL "ggml: enable AVX"         FORCE)
set(GGML_AVX2         OFF CACHE BOOL "ggml: enable AVX2"        FORCE)
set(GGML_AVX_VNNI     OFF CACHE BOOL "ggml: enable AVX-VNNI"    FORCE)
set(GGML_AVX512       OFF CACHE BOOL "ggml: enable AVX512F"     FORCE)
set(GGML_AVX512_VNNI  OFF CACHE BOOL "ggml: enable AVX512-VNNI" FORCE)
set(GGML_AVX512_VBMI  OFF CACHE BOOL "ggml: enable AVX512-VBMI" FORCE)
set(GGML_AVX512_BF16  OFF CACHE BOOL "ggml: enable AVX512-BF16" FORCE)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    if(JOB_AVX_512_VNNI_FLAG)
        set(GGML_AVX          ON CACHE BOOL "ggml: enable AVX"          FORCE)
        set(GGML_AVX2         ON CACHE BOOL "ggml: enable AVX2"         FORCE)
        set(GGML_AVX512       ON CACHE BOOL "ggml: enable AVX512F"      FORCE)
        set(GGML_AVX512_VNNI  ON CACHE BOOL "ggml: enable AVX512-VNNI"  FORCE)
    elseif(JOB_AVX_512_FLAG)
        set(GGML_AVX          ON CACHE BOOL "ggml: enable AVX"          FORCE)
        set(GGML_AVX2         ON CACHE BOOL "ggml: enable AVX2"         FORCE)
        set(GGML_AVX512       ON CACHE BOOL "ggml: enable AVX512F"      FORCE)
    elseif(JOB_AVX_VNNI_FLAG)
        set(GGML_AVX          ON CACHE BOOL "ggml: enable AVX"          FORCE)
        set(GGML_AVX2         ON CACHE BOOL "ggml: enable AVX2"         FORCE)
        set(GGML_AVX_VNNI     ON CACHE BOOL "ggml: enable AVX-VNNI"     FORCE)
    elseif(JOB_AVX_TWO_FLAG)
        set(GGML_AVX          ON CACHE BOOL "ggml: enable AVX"          FORCE)
        set(GGML_AVX2         ON CACHE BOOL "ggml: enable AVX2"         FORCE)
    elseif(JOB_AVX_FLAG)
        set(GGML_AVX          ON CACHE BOOL "ggml: enable AVX"          FORCE)
    endif()
endif()

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

    if(JOB_CUDA)
        set(NV_VULKAN_SEARCH_PATHS
            "/usr/share/vulkan/icd.d/nvidia_icd.json"
            "/etc/vulkan/icd.d/nvidia_icd.json"
            "/usr/local/share/vulkan/icd.d/nvidia_icd.json"
        )

        set(NV_ICD_FOUND FALSE)
        foreach(ICD_PATH IN LISTS NV_VULKAN_SEARCH_PATHS)
            if(EXISTS "${ICD_PATH}")
                set(NV_ICD_FOUND TRUE)
                break()
            endif()
        endforeach()

        if(NOT NV_ICD_FOUND)
            message(FATAL_ERROR
                "JOB_CUDA and GGML_VULKAN are enabled, but no NVIDIA Vulkan ICD manifest "
                "(nvidia_icd.json) was found. Please install 'nvidia-vulkan-icd' or ensure "
                "the driver ICD manifest is present in /usr/share/vulkan/icd.d or /etc/vulkan/icd.d."
            )
        endif()
    endif()
endif()

## Enables the RPC backend.
set(GGML_RPC ON)

## Changes the upstream OpenCL Adreno-kernel default from ON to OFF.
set(GGML_OPENCL_USE_ADRENO_KERNELS OFF)

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
