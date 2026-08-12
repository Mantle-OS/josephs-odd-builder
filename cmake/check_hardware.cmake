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
    include(cmake/win32/check_hardware_win32.cmake)
elseif(APPLE)
    set(JOB_OSX ON)
    add_compile_definitions(JOB_OSX)
    include(cmake/osx/check_hardware_osx.cmake)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(JOB_LINUX ON)
    add_compile_definitions(JOB_LINUX)
    include(cmake/linux/check_hardware_linux.cmake)  ## HERE this is new
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    set(JOB_FREE_BSD ON)
    add_compile_definitions(JOB_FREE_BSD)
    include(cmake/bsd/check_hardware_bsd.cmake)
elseif(CMAKE_SYSTEM_NAME MATCHES "^(OpenBSD|NetBSD|DragonFly)$")
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is currently not supported due to developer effort."
        "However, pull requests are welcome and there is a porting guide located in the docs/job/bsd.md file.")
else()
    message(FATAL_ERROR "Unsupported target operating system: ${CMAKE_SYSTEM_NAME}")
endif()


check_ipo_supported(RESULT has_ipo OUTPUT output)
if(has_ipo)
    message(STATUS "IPO/LTO is supported: enabling globally")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    message(WARNING "IPO/LTO is not supported: ${output}")
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

math(EXPR RAW_BC "(${JOB_AI_L2_TILE_BUDGET} / 2) / (${JOB_AI_HEAD_DIM} * ${JOB_AI_ELEMENT_BYTES})")

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
