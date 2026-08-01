add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-O3>)


check_cxx_compiler_flag("-fno-omit-frame-pointer" COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
if(COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
    add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-fno-omit-frame-pointer>)
endif()

check_cxx_compiler_flag("-funroll-loops" COMPILER_SUPPORTS_UNROLL_LOOPS)
if(COMPILER_SUPPORTS_UNROLL_LOOPS)
    add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-funroll-loops>)
endif()

check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    list(APPEND JOB_CXX_FLAGS "-march=native")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-march=native>)
endif()

# FAST MATH
check_cxx_compiler_flag("-ffast-math" CXX_SUPPORTS_FAST_MATH_FLAG)
if(CXX_SUPPORTS_FAST_MATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-ffast-math")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ffast-math>)
endif()

# Fused Multiply Add
check_cxx_compiler_flag("-mfma" CXX_SUPPORTS_FMATH_FLAG)
if(CXX_SUPPORTS_FMATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-mfma")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mfma>)
endif()

## Start Looking for the higest SIMD Implemention based on the processor type
## I tried to use check_cxx_compiler_flag for this and it turned out to not work in some cases even though it should have so that is why this is here
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    #############################################
    # 16-width kernels
    #############################################
    # AVX_VINNI
    if(JOB_AVX_512_VNNI_FLAG)
        add_compile_definitions(HAS_AVX_512_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavx512vnni")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx512vnni>)
    # AVX_512
    elseif(JOB_AVX_512_FLAG)
        add_compile_definitions(HAS_AVX_512)
        list(APPEND JOB_CXX_FLAGS "-mavx512f")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx512f>)
    #############################################
    # 8 width kernels
    #############################################
    elseif(JOB_AVX_VNNI_FLAG)
        add_compile_definitions(HAS_AVX_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavxvnni")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavxvnni>)
    # AVX2
    elseif(JOB_AVX_TWO_FLAG)
        add_compile_definitions(HAS_AVX_TWO)
        list(APPEND JOB_CXX_FLAGS "-mavx2")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx2>)
    ## AVX
    elseif(JOB_AVX_FLAG)
        add_compile_definitions(HAS_AVX)
        list(APPEND JOB_CXX_FLAGS "-mavx")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx>)

    #############################################
    # 4 width kernels
    #############################################
    # could add SSE support or whatever maybe later for older chipsets
    endif()

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    add_compile_definitions(HAS_NEON)
    list(APPEND JOB_CXX_FLAGS "-mneon")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mneon>)
endif()
