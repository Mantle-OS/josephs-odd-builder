# macOS compiler flags and optimization checks (Apple Clang)

add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-O3>)

add_compile_options(
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
)

# Frame Pointer
check_cxx_compiler_flag("-fno-omit-frame-pointer" COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
if(COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
    add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-fno-omit-frame-pointer>)
endif()

# Unroll Loops
check_cxx_compiler_flag("-funroll-loops" COMPILER_SUPPORTS_UNROLL_LOOPS)
if(COMPILER_SUPPORTS_UNROLL_LOOPS)
    add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-funroll-loops>)
endif()

# Fast Math
check_cxx_compiler_flag("-ffast-math" CXX_SUPPORTS_FAST_MATH_FLAG)
if(CXX_SUPPORTS_FAST_MATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-ffast-math")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ffast-math>)
endif()

# FMA
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    check_cxx_compiler_flag("-mfma" CXX_SUPPORTS_FMATH_FLAG)
    if(CXX_SUPPORTS_FMATH_FLAG)
        list(APPEND JOB_CXX_FLAGS "-mfma")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mfma>)
    endif()

    check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
    if(COMPILER_SUPPORTS_MARCH_NATIVE)
        list(APPEND JOB_CXX_FLAGS "-march=native")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-march=native>)
    endif()
endif()

# SIMD Capabilities for macOS (Apple Clang)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    if(JOB_AVX_512_VNNI_FLAG)
        message(STATUS "Highest AVX is AVX 512 VNNI")
        add_compile_definitions(HAS_AVX_512_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavx512vnni")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx512vnni>)
    elseif(JOB_AVX_512_FLAG)
        message(STATUS "Highest AVX is AVX 512 F")
        add_compile_definitions(HAS_AVX_512)
        list(APPEND JOB_CXX_FLAGS "-mavx512f")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx512f>)
    elseif(JOB_AVX_VNNI_FLAG)
        message(STATUS "Highest AVX is VNNI")
        add_compile_definitions(HAS_AVX_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavxvnni")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavxvnni>)
    elseif(JOB_AVX_TWO_FLAG)
        message(STATUS "Highest AVX is AVX 2")
        add_compile_definitions(HAS_AVX_TWO)
        list(APPEND JOB_CXX_FLAGS "-mavx2")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx2>)
    elseif(JOB_AVX_FLAG)
        message(STATUS "Highest AVX is AVX")
        add_compile_definitions(HAS_AVX)
        list(APPEND JOB_CXX_FLAGS "-mavx")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx>)
    endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    message(STATUS "Apple Silicon detected: NEON is built-in")
    add_compile_definitions(HAS_NEON)
endif()
