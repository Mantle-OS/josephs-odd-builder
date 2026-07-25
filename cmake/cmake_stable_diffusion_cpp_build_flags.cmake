## STABLE_DIFFUSION_CPP
set(SD_BUILD_EXAMPLES OFF)
set(SD_USE_SYSTEM_WEBM OFF)
set(SD_USE_SYSTEM_WEBP ON)
set(SD_CUDA ON)
set(SD_VULKAN ON)
set(SD_OPENCL ON)

include(${CMAKE_CURRENT_LIST_DIR}/cmake_ggml_build_flags.cmake)

add_subdirectory(3rdparty/stable-diffusion.cpp)

if(TARGET ggml-cpu)
    target_compile_options(ggml-cpu PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>
    )
endif()

if (TARGET ggml)
    target_compile_options(ggml PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>
    )
endif()
if (TARGET ggml-base)
    target_compile_options(ggml-base PRIVATE
        $<$<COMPILE_LANGUAGE:C>:-fno-finite-math-only>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-finite-math-only>
    )
endif()