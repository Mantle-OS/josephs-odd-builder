include(CheckCXXCompilerFlag)
include(CheckSourceCompiles)
include(CheckIPOSupported)

add_compile_options(
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-funroll-loops>
    $<$<CONFIG:Release>:-fno-omit-frame-pointer>
)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

check_ipo_supported(RESULT has_ipo OUTPUT output)
if(has_ipo)
    message(STATUS "IPO/LTO is supported: enabling globally")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    message(WARNING "IPO/LTO is not supported: ${output}")
endif()

set(JOB_CXX_FLAGS)


# 32 rows * 64 dim * 4 bytes = 8KB. => L1 cache
add_compile_definitions(JOB_AI_BLOCK_ROWS=32)
# 128 cols * 64 dim * 4 bytes = 32KB. Fits in L1/L2.
add_compile_definitions(JOB_AI_BLOCK_COLS=128)
# "standard" transformer size
add_compile_definitions(JOB_AI_HEAD_DIM=64)

## NATIVE
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "Enabling host architecture tuning (-march=native)")
    add_compile_options($<$<CONFIG:Release>:-march=native>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-march=native>)
    add_compile_options($<$<CONFIG:Debug>:-march=native>)
endif()

## FAST MATHH
check_cxx_compiler_flag("-ffast-math" CXX_SUPPORTS_FAST_MATH_FLAG)
if(CXX_SUPPORTS_FAST_MATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-ffast-math")
    add_compile_options($<$<CONFIG:Release>:-ffast-math>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-ffast-math>)
    add_compile_options($<$<CONFIG:Debug>:-ffast-math>)
endif()

check_cxx_compiler_flag("-mfma" CXX_SUPPORTS_FMATH_FLAG)
if(CXX_SUPPORTS_FMATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-mfma")
    add_compile_options($<$<CONFIG:Release>:-mfma>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-mfma>)
    add_compile_options($<$<CONFIG:Debug>:-mfma>)
endif()


if(NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")    
    #############################################
    # 16-width kernels
    #############################################
    # AVX_VINNI
    if(JOB_AVX_512_VNNI_FLAG)
        message(STATUS "Highest AVX is AVX 512 VNNI")
        add_compile_definitions(HAS_AVX_512_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavx512vnni")
        add_compile_options($<$<CONFIG:Release>:-mavx512vnni>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mavx512vnni>)
        add_compile_options($<$<CONFIG:Debug>:-mavx512vnni>)

    # AVX_512
    elseif(JOB_AVX_512_FLAG)
        message(STATUS "Highest AVX is AVX 512 F")
        add_compile_definitions(HAS_AVX_512)
        list(APPEND JOB_CXX_FLAGS "-mavx512f")
        add_compile_options($<$<CONFIG:Release>:-mavx512f>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mavx512f>)
        add_compile_options($<$<CONFIG:Debug>:-mavx512f>)

    #############################################
    # 8 width kernels
    #############################################
    elseif(JOB_AVX_VNNI_FLAG)
        message(STATUS "Highest AVX is VNNI")
        add_compile_definitions(HAS_AVX_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavxvnni")
        add_compile_options($<$<CONFIG:Release>:-mavxvnni>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mavxvnni>)
        add_compile_options($<$<CONFIG:Debug>:-mavxvnni>)
    # AVX2
    elseif(JOB_AVX_TWO_FLAG)
        message(STATUS "Highest AVX is AVX 2")
        add_compile_definitions(HAS_AVX_TWO)
        list(APPEND JOB_CXX_FLAGS "-mavx2")
        add_compile_options($<$<CONFIG:Release>:-mavx2>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mavx2>)
        add_compile_options($<$<CONFIG:Debug>:-mavx2>)

    ## AVX
    elseif(JOB_AVX_FLAG)
        message(STATUS "Highest AVX is AVX")
        add_compile_definitions(HAS_AVX)
        list(APPEND JOB_CXX_FLAGS "-mavx")
        add_compile_options($<$<CONFIG:Release>:-mavx>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mavx>)
        add_compile_options($<$<CONFIG:Debug>:-mavx>)

    #############################################
    # 4 width kernels
    #############################################
    # could add SSE support or whatever maybe later for older chipsets
    endif()


##END X86_64


elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    add_compile_definitions(JOB_BLOCK_SIZE=128)
    add_compile_definitions(JOB_L3_PER_CCD_BYTES=33554432) # 32MB
    add_compile_definitions(JOB_DEFAULT_WS_MB=256)         # Safe default
    check_cxx_compiler_flag("-mneon" CXX_SUPPORTS_NEON_FLAG)
    if(CXX_SUPPORTS_NEON_FLAG)
        message(STATUS "Highest AVX is NEON")
        add_compile_definitions(HAS_NEON)
        list(APPEND JOB_CXX_FLAGS "-mneon")
        add_compile_options($<$<CONFIG:Release>:-mneon>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mneon>)
        add_compile_options($<$<CONFIG:Debug>:-mneon>)
    endif()
endif()
