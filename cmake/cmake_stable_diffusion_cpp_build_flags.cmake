## STABLE_DIFFUSION_CPP
set(SD_BUILD_EXAMPLES OFF)
set(SD_USE_SYSTEM_WEBM OFF)
set(SD_USE_SYSTEM_WEBP ON)
set(SD_CUDA ON)
set(SD_VULKAN ON)
set(SD_OPENCL ON)

include(${CMAKE_CURRENT_LIST_DIR}/cmake_ggml_build_flags.cmake)
add_subdirectory(3rdparty/stable-diffusion.cpp)

# we know have access to the targets
if(TARGET ggml-cpu)
    set_target_properties(ggml-cpu PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    target_compile_options(ggml-cpu PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>
    )
endif()

if (TARGET ggml)
    set_target_properties(ggml PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    target_compile_options(ggml PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>
    )
endif()
if (TARGET ggml-base)
    set_target_properties(ggml-base PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
    target_compile_options(ggml-base PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>
    )
endif()

if(TARGET ggml-vulkan)
    set_target_properties(ggml-vulkan PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()
if(TARGET ggml-opencl)
    set_target_properties(ggml-opencl PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()
if(TARGET ggml-blas)
    set_target_properties(ggml-blas PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()

if(TARGET ggml-cuda)
    set_target_properties(ggml-cuda PROPERTIES
        POSITION_INDEPENDENT_CODE ON
    )
endif()
