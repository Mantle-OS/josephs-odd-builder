include(CheckCXXCompilerFlag)
include(CheckSourceCompiles)

set(JOB_CXX_FLAGS)


# 32 rows * 64 dim * 4 bytes = 8KB. => L1 cache
add_compile_definitions(JOB_AI_BLOCK_ROWS=32)
# 128 cols * 64 dim * 4 bytes = 32KB. Fits in L1/L2.
add_compile_definitions(JOB_AI_BLOCK_COLS=128)
# "standard" transformer size
add_compile_definitions(JOB_AI_HEAD_DIM=64)

check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "Enabling host architecture tuning (-march=native)")
    add_compile_options($<$<CONFIG:Release>:-march=native>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-march=native>)
    add_compile_options($<$<CONFIG:Debug>:-march=native>)
endif()

check_cxx_compiler_flag("-ffast-math" CXX_SUPPORTS_FAST_MATH_FLAG)
if(CXX_SUPPORTS_FAST_MATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-ffast-math")
    add_compile_options($<$<CONFIG:Release>:-ffast-math>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-ffast-math>)
    add_compile_options($<$<CONFIG:Debug>:-ffast-math>)
endif()


check_cxx_compiler_flag("-mavx2" CXX_SUPPORTS_AVX_TWO_FLAG)
if(CXX_SUPPORTS_AVX_TWO_FLAG)
    list(APPEND JOB_CXX_FLAGS "-mavx2")
    add_compile_options($<$<CONFIG:Release>:-mavx2>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-mavx2>)
    add_compile_options($<$<CONFIG:Debug>:-mavx2>)
endif()

check_cxx_compiler_flag("-mfma" CXX_SUPPORTS_FMATH_FLAG)
if(CXX_SUPPORTS_FMATH_FLAG)
    list(APPEND JOB_CXX_FLAGS "-mfma")
    add_compile_options($<$<CONFIG:Release>:-mfma>)
    add_compile_options($<$<CONFIG:RelWithDebInfo>:-mfma>)
    add_compile_options($<$<CONFIG:Debug>:-mfma>)
endif()


# check_cxx_compiler_flag("-AVX vector return without AVX enabled changes the ABI [-Werror=psabi]" CXX_SUPPORTS_AVX_FLAG)


if(NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    check_cxx_compiler_flag("-mavx" CXX_SUPPORTS_AVX_FLAG)
    if(CXX_SUPPORTS_AVX_FLAG)
        add_compile_definitions(HAS_AVX)
        list(APPEND JOB_CXX_FLAGS "-mavx")
        add_compile_options($<$<CONFIG:Release>:-mavx>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mavx>)
        add_compile_options($<$<CONFIG:Debug>:-mavx>)
    endif()

    option(JOB_BLOCK_SIZE_256 "64Mb L3 Build" OFF)
    if(JOB_BLOCK_SIZE_256)
        add_compile_definitions(JOB_RYZEN_5950X)
        add_compile_definitions(JOB_BLOCK_SIZE=256)
    else()
        add_compile_definitions(JOB_BLOCK_SIZE=128)
    endif()

    # Define the Macro-Memory Physics (Hardware Caps)
    if(JOB_BLOCK_SIZE_256)
        add_compile_definitions(JOB_L3_PER_CCD_BYTES=33554432) # 32MB
        add_compile_definitions(JOB_DEFAULT_WS_MB=512)         # Big workspace
    else()
        add_compile_definitions(JOB_L3_PER_CCD_BYTES=33554432) # 32MB
        add_compile_definitions(JOB_DEFAULT_WS_MB=256)         # Safe default
    endif()


elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    add_compile_definitions(JOB_BLOCK_SIZE=128)
    add_compile_definitions(JOB_L3_PER_CCD_BYTES=33554432) # 32MB
    add_compile_definitions(JOB_DEFAULT_WS_MB=256)         # Safe default
    check_cxx_compiler_flag("-mneon" CXX_SUPPORTS_NEON_FLAG)
    if(CXX_SUPPORTS_NEON_FLAG)
        add_compile_definitions(HAS_NEON)
        list(APPEND JOB_CXX_FLAGS "-mneon")
        add_compile_options($<$<CONFIG:Release>:-mneon>)
        add_compile_options($<$<CONFIG:RelWithDebInfo>:-mneon>)
        add_compile_options($<$<CONFIG:Debug>:-mneon>)
    endif()
endif()
