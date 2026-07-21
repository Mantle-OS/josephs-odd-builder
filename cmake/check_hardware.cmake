include(CheckCXXCompilerFlag)
include(CheckSourceCompiles)
include(CheckIPOSupported)


set(JOB_WINDOWS OFF)
set(JOB_LINUX OFF)
set(JOB_APPLE OFF)
set(JOB_BSD OFF)

if(WIN32)
    set(JOB_WINDOWS ON)
    add_compile_definitions(JOB_WINDOWS)
elseif(APPLE)
    set(JOB_APPLE ON)
    add_compile_definitions(JOB_APPLE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(JOB_LINUX ON)
    add_compile_definitions(JOB_LINUX)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(FreeBSD|OpenBSD|NetBSD|DragonFly)$")
    set(JOB_BSD ON)
    add_compile_definitions(JOB_BSD)
else()
    message(FATAL_ERROR
        "Unsupported target operating system: ${CMAKE_SYSTEM_NAME}"
    )
endif()

add_compile_options(
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-O3>
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-funroll-loops>
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-fno-omit-frame-pointer>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
)
# $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
# $<$<COMPILE_LANGUAGE:CXX>:-Werror>

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

# NATIVE
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "Enabling host architecture tuning (-march=native)")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-march=native>)
endif()

# FAST MATHH
check_cxx_compiler_flag("-ffast-math" CXX_SUPPORTS_FAST_MATH_FLAG)
if(CXX_SUPPORTS_FAST_MATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-ffast-math")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ffast-math>)
endif()

check_cxx_compiler_flag("-mfma" CXX_SUPPORTS_FMATH_FLAG)
if(CXX_SUPPORTS_FMATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-mfma")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mfma>)
endif()

# ... MSVC gonna catch this IDK ?
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    #############################################
    # 16-width kernels
    #############################################
    # AVX_VINNI
    if(JOB_AVX_512_VNNI_FLAG)
        message(STATUS "Highest AVX is AVX 512 VNNI")
        add_compile_definitions(HAS_AVX_512_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavx512vnni")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx512vnni>)
    # AVX_512
    elseif(JOB_AVX_512_FLAG)
        message(STATUS "Highest AVX is AVX 512 F")
        add_compile_definitions(HAS_AVX_512)
        list(APPEND JOB_CXX_FLAGS "-mavx512f")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx512f>)
    #############################################
    # 8 width kernels
    #############################################
    elseif(JOB_AVX_VNNI_FLAG)
        message(STATUS "Highest AVX is VNNI")
        add_compile_definitions(HAS_AVX_VNNI)
        list(APPEND JOB_CXX_FLAGS "-mavxvnni")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavxvnni>)
    # AVX2
    elseif(JOB_AVX_TWO_FLAG)
        message(STATUS "Highest AVX is AVX 2")
        add_compile_definitions(HAS_AVX_TWO)
        list(APPEND JOB_CXX_FLAGS "-mavx2")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx2>)
    ## AVX
    elseif(JOB_AVX_FLAG)
        message(STATUS "Highest AVX is AVX")
        add_compile_definitions(HAS_AVX)
        list(APPEND JOB_CXX_FLAGS "-mavx")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mavx>)

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
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mneon>)
    endif()
endif()
