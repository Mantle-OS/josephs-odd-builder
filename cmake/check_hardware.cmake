include(CheckCXXCompilerFlag)
include(CheckSourceCompiles)
include(CheckIPOSupported)

set(JOB_WINDOWS  OFF)
set(JOB_LINUX    OFF)
set(JOB_OSX      OFF)
set(JOB_FREE_BSD OFF)
set(JOB_CXX_FLAGS)

if(WIN32)
    # add checks to make sure it is msvc
    set(JOB_WINDOWS ON)
    add_compile_definitions(JOB_WINDOWS)
elseif(APPLE)
    set(JOB_OSX ON)
    add_compile_definitions(JOB_OSX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(JOB_LINUX ON)
    add_compile_definitions(JOB_LINUX)
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(JOB_FREE_BSD ON)
    add_compile_definitions(JOB_FREE_BSD)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(OpenBSD|NetBSD|DragonFly)$")
    ## FIXME make the document.
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is currently not supported due to developer effort."
        "However, pull requests are welcome and there is a porting guide located in the docs/job/bsd.md file.")
else()
    message(FATAL_ERROR "Unsupported target operating system: ${CMAKE_SYSTEM_NAME}")
endif()

# check_cxx_compiler_flag("-std=c++11" SUPPORTS_CXX11)
# check_cxx_compiler_flag("-std=c++17" SUPPORTS_CXX17)
# check_cxx_compiler_flag("-std=c++20" SUPPORTS_CXX20)
# check_cxx_compiler_flag("-std=c++23" SUPPORTS_CXX23)
# check_cxx_compiler_flag("-std=c++26" SUPPORTS_CXX26)

## Already cross platform
check_ipo_supported(RESULT has_ipo OUTPUT output)
if(has_ipo)
    message(STATUS "IPO/LTO is supported: enabling globally")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    message(WARNING "IPO/LTO is not supported: ${output}")
endif()

add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-O3>)

if(JOB_WINDOWS)
    ## Lets see how this even works
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    )
else()
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
        $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    )
## Thse got nerf'd when ggml, llama-cpp and sd-cpp
# $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
# $<$<COMPILE_LANGUAGE:CXX>:-Werror>
endif()


## FIXME update function to check per OS that we know we are compiling for
check_cxx_compiler_flag("-fno-omit-frame-pointer" COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
if(COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER)
    add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-fno-omit-frame-pointer>)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Adding unroll loops if in release build")
    endif()
endif()

## FIXME update function to check per OS that we know we are compiling for
check_cxx_compiler_flag("-funroll-loops" COMPILER_SUPPORTS_UNROLL_LOOPS)
if(COMPILER_SUPPORTS_UNROLL_LOOPS)
    add_compile_options($<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:-funroll-loops>)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Adding no omit frame pointer if in release build")
    endif()
endif()


## FIXME update function to check per OS that we know we are compiling for
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    message(STATUS "Enabling host architecture tuning (-march=native)")
    list(APPEND JOB_CXX_FLAGS "-march=native")

    ## FIXME maybe only on release ?
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-march=native>)
endif()


# FAST MATH
## FIXME update function to check per OS that we know we are compiling for
check_cxx_compiler_flag("-ffast-math" CXX_SUPPORTS_FAST_MATH_FLAG)
if(CXX_SUPPORTS_FAST_MATH_FLAG)
    message(STATUS "Enabling fast math (-ffast-math)")
    list(APPEND JOB_CXX_FLAGS "-ffast-math")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ffast-math>)
endif()

# Fused Multiply Add
## FIXME update function to check per OS that we know we are compiling for
check_cxx_compiler_flag("-mfma" CXX_SUPPORTS_FMATH_FLAG)
if(CXX_SUPPORTS_FMATH_FLAG)
        message(STATUS "Enabling fused multiply add (-mfma)")
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

elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|aarch64)")
    # Historical ARM tuning used: JOB_BLOCK_SIZE=128 JOB_DEFAULT_WS_MB=256
    # For the ARM boards I have and the new cache-derived configuration, Change JOB_L3_MB=16. to match up
    # The old configuration did not encode L1 or L2 cache sizes, so
    # JOB_L1_KB and JOB_L2_KB must come from the actual target/toolchain.
    message(STATUS "Highest AVX is NEON")
    add_compile_definitions(HAS_NEON)
    list(APPEND JOB_CXX_FLAGS "-mneon")
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-mneon>)
endif()


## Now that SIMD is setup....
##############################
# AI Tile Configuration
##############################

set(JOB_AI_HEAD_DIM "64" CACHE STRING "Transformer attention head dimension")

# Keep the stack accumulator synchronized with the configured head dim. Runtime dim above this value use the heap path.
set(JOB_AI_MAX_STACK_DIM "${JOB_AI_HEAD_DIM}")

set(JOB_AI_ELEMENT_BYTES 4)

# Target part of each cache to leave capacity for other hot data, temporary values, cache conflicts, prefetching, and register spills.
set(JOB_AI_L1_USAGE_PERCENT 50)
set(JOB_AI_L2_USAGE_PERCENT 50)

math(EXPR JOB_L1_BYTES "${JOB_L1_KB} * 1024")
math(EXPR JOB_L2_BYTES "${JOB_L2_KB} * 1024")
math(EXPR JOB_L3_BYTES "${JOB_L3_MB} * 1024 * 1024")
math(EXPR JOB_AI_L1_TILE_BUDGET "${JOB_L1_BYTES} * ${JOB_AI_L1_USAGE_PERCENT} / 100")
math(EXPR JOB_AI_L2_TILE_BUDGET "${JOB_L2_BYTES} * ${JOB_AI_L2_USAGE_PERCENT} / 100")

##############################
# Column Tile
##############################
# Keep the active K and V tiles within the selected L2 budget:
#
#   K tile = BLOCK_COLS * HEAD_DIM * ELEMENT_BYTES
#   V tile = BLOCK_COLS * HEAD_DIM * ELEMENT_BYTES
math(EXPR RAW_BC "(${JOB_AI_L2_TILE_BUDGET} / 2) / (${JOB_AI_HEAD_DIM} * ${JOB_AI_ELEMENT_BYTES})")

# Select the largest supported column tile that fits the
# cache-derived limit. The validated ceiling is currently 256.
if(RAW_BC GREATER_EQUAL 256)
    set(JOB_AI_BLOCK_COLS 256)
elseif(RAW_BC GREATER_EQUAL 128)
    set(JOB_AI_BLOCK_COLS 128)
elseif(RAW_BC GREATER_EQUAL 64)
    set(JOB_AI_BLOCK_COLS 64)
elseif(RAW_BC GREATER_EQUAL 32)
    set(JOB_AI_BLOCK_COLS 32)
else()
    message(FATAL_ERROR "The selected L2 budget cannot hold the minimum 32-column K/V tile. Calculated maximum: ${RAW_BC}")
endif()

##############################
# Row Tile
##############################
# Approximate hot L1 footprint per query row:
#   Q row              = HEAD_DIM
#   output accumulator = MAX_STACK_DIM
#   score row          = BLOCK_COLS
# P_row, l_local, and m_local are small fixed additions outside this
# per-row expression.
math(EXPR ROW_FLOATS_FOOTPRINT
    "${JOB_AI_HEAD_DIM} +
     ${JOB_AI_MAX_STACK_DIM} +
     ${JOB_AI_BLOCK_COLS}"
)

math(EXPR ROW_BYTES_FOOTPRINT "${ROW_FLOATS_FOOTPRINT} * ${JOB_AI_ELEMENT_BYTES}")
math(EXPR JOB_AI_FIXED_L1_BYTES "${JOB_AI_BLOCK_COLS} * ${JOB_AI_ELEMENT_BYTES}")
math(EXPR JOB_AI_L1_ROW_BUDGET "${JOB_AI_L1_TILE_BUDGET} - ${JOB_AI_FIXED_L1_BYTES}")

if(JOB_AI_L1_ROW_BUDGET LESS_EQUAL 0)
    message(FATAL_ERROR "The selected L1 budget cannot hold P_row.")
endif()

math(EXPR RAW_BR "${JOB_AI_L1_ROW_BUDGET} / ${ROW_BYTES_FOOTPRINT}")

if(RAW_BR GREATER_EQUAL 64)
    set(JOB_AI_BLOCK_ROWS 64)
elseif(RAW_BR GREATER_EQUAL 32)
    set(JOB_AI_BLOCK_ROWS 32)
elseif(RAW_BR GREATER_EQUAL 16)
    set(JOB_AI_BLOCK_ROWS 16)
elseif(RAW_BR GREATER_EQUAL 8)
    set(JOB_AI_BLOCK_ROWS 8)
else()
    message(FATAL_ERROR "The selected L1 budget cannot hold the minimum 8-row tile. Calculated maximum: ${RAW_BR}")
endif()

if(JOB_L3_MB STREQUAL "32")
    set(JOB_BLOCK_SIZE 256)
    set(JOB_DEFAULT_WS_MB 512)
elseif(JOB_L3_MB STREQUAL "16")
    set(JOB_BLOCK_SIZE 128)
    set(JOB_DEFAULT_WS_MB 256)
elseif(JOB_L3_MB STREQUAL "8")
    set(JOB_BLOCK_SIZE 64)
    set(JOB_DEFAULT_WS_MB 128)
else()
    # [[BACKLOG]] Add support for a 64 MiB L3 cache domain.
    message(FATAL_ERROR "JOB_L3_MB must be 8, 16, or 32. Provided: ${JOB_L3_MB}"
    )
endif()

add_compile_definitions(
    JOB_AI_HEAD_DIM=${JOB_AI_HEAD_DIM}
    JOB_AI_MAX_STACK_DIM=${JOB_AI_MAX_STACK_DIM}
    JOB_AI_ELEMENT_BYTES=${JOB_AI_ELEMENT_BYTES}
    JOB_AI_BLOCK_ROWS=${JOB_AI_BLOCK_ROWS}
    JOB_AI_BLOCK_COLS=${JOB_AI_BLOCK_COLS}

    JOB_L1_PER_CORE_BYTES=${JOB_L1_BYTES}
    JOB_L2_PER_CORE_BYTES=${JOB_L2_BYTES}
    JOB_L3_DOMAIN_BYTES=${JOB_L3_BYTES}

    JOB_BLOCK_SIZE=${JOB_BLOCK_SIZE}
    JOB_DEFAULT_WS_MB=${JOB_DEFAULT_WS_MB}
)

message(STATUS "")
message(STATUS "JOB Hardware and Build Configuration")
message(STATUS "============================================================")

message(STATUS "Target")
message(STATUS "  Operating system:       ${CMAKE_SYSTEM_NAME}")
message(STATUS "  System processor:       ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "  Host operating system:  ${CMAKE_HOST_SYSTEM_NAME}")
message(STATUS "  Host processor:         ${CMAKE_HOST_SYSTEM_PROCESSOR}")
message(STATUS "  Cross compiling:        ${CMAKE_CROSSCOMPILING}")

message(STATUS "------------------------------------------------------------")

message(STATUS "Compiler")
message(STATUS "  C++ compiler:           ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Compiler version:       ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Compiler path:          ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ standard:           C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type:             ${CMAKE_BUILD_TYPE}")
message(STATUS "  IPO/LTO enabled:        ${CMAKE_INTERPROCEDURAL_OPTIMIZATION}")
message(STATUS "  Resolved C++ flags:     ${JOB_CXX_FLAGS}")

message(STATUS "------------------------------------------------------------")

message(STATUS "Compiler Flag Support")
message(STATUS "  -march=native:          ${COMPILER_SUPPORTS_MARCH_NATIVE}")
message(STATUS "  -ffast-math:            ${CXX_SUPPORTS_FAST_MATH_FLAG}")
message(STATUS "  -mfma:                  ${CXX_SUPPORTS_FMATH_FLAG}")
message(STATUS "  -funroll-loops:         ${COMPILER_SUPPORTS_UNROLL_LOOPS}")
message(STATUS "  frame pointer:          ${COMPILER_SUPPORTS_NO_OMIT_FRAME_POINTER}")
message(STATUS "  NEON flag:              ${CXX_SUPPORTS_NEON_FLAG}")

message(STATUS "------------------------------------------------------------")

message(STATUS "Cache Configuration")
message(STATUS "  L1 data cache:          ${JOB_L1_KB} KiB")
message(STATUS "  L2 cache:               ${JOB_L2_KB} KiB")
message(STATUS "  L3 cache domain:        ${JOB_L3_MB} MiB")
message(STATUS "  L1 tile budget:         ${JOB_AI_L1_TILE_BUDGET} bytes")
message(STATUS "  L2 tile budget:         ${JOB_AI_L2_TILE_BUDGET} bytes")

message(STATUS "------------------------------------------------------------")

message(STATUS "General Compute")
message(STATUS "  General block size:     ${JOB_BLOCK_SIZE}")
message(STATUS "  Default workspace:      ${JOB_DEFAULT_WS_MB} MiB")

message(STATUS "------------------------------------------------------------")

message(STATUS "AI Tile Configuration")
message(STATUS "  Attention tile:         ${JOB_AI_BLOCK_ROWS}x${JOB_AI_BLOCK_COLS}")
message(STATUS "  Head dimension:         ${JOB_AI_HEAD_DIM}")
message(STATUS "  Maximum stack dim:      ${JOB_AI_MAX_STACK_DIM}")
message(STATUS "  Element size:           ${JOB_AI_ELEMENT_BYTES} bytes")
message(STATUS "  Raw column limit:       ${RAW_BC}")
message(STATUS "  Raw row limit:          ${RAW_BR}")

message(STATUS "------------------------------------------------------------")

message(STATUS "Optional Components")
message(STATUS "  CUDA support:           ${JOB_CUDA}")
message(STATUS "  Qt libs and apps:       ${JOB_QT}")
message(STATUS "  MsgPack bindings:       ${JOB_BUILD_SERIALIZER_MSGPACK}")
message(STATUS "  FlatBuffers bindings:   ${JOB_BUILD_SERIALIZER_FLATBUFFERS}")
message(STATUS "  CI configuration:       ${JOB_CI_BUILD}")
message(STATUS "  Test benchmarks:        ${JOB_TEST_BENCHMARKS}")

message(STATUS "============================================================")
message(STATUS "")

