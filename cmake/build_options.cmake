include(CheckCXXCompilerFlag)
##############################
# Compiler flags
##############################
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_QT_CREATOR_SKIP_MAINTENANCE_TOOL_PROVIDER ON)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW) # for reproducible timestamps
endif()

##############################
# AVX Settings
##############################
option(JOB_AVX_FLAG           "build with avx" OFF)
option(JOB_AVX_2_FLAG         "build with avx two" OFF)
option(JOB_AVX_VNNI_FLAG      "build with avx vnni" ON)
option(JOB_AVX_512_FLAG       "build with avx 512f" OFF)
option(JOB_AVX_512_VNNI_FLAG  "build with avx 512vnni" OFF)
option(JOB_AVX_NEON_FLAG      "build with avx neon" OFF)

##############################
# Options
##############################
set(JOB_L3_MB "32" CACHE STRING "L3 Cache size per CCD in Megabytes")
if(NOT JOB_L3_MB MATCHES "^(32|16|8)$")
    message(FATAL_ERROR "JOB_L3_MB must be 8, 16, or 32! You provided: ${JOB_L3_MB}")
endif()

# add_compile_definitions(JOB_L3_PER_CCD_BYTES=67108864) # 64MB Future
if(JOB_L3_MB STREQUAL "32")
    add_compile_definitions(JOB_BLOCK_SIZE=256)
    add_compile_definitions(JOB_L3_PER_CCD_BYTES=33554432) # 32MB
    add_compile_definitions(JOB_DEFAULT_WS_MB=512)         # Bigger workspace
elseif(JOB_L3_MB STREQUAL "16")
    add_compile_definitions(JOB_BLOCK_SIZE=128)
    add_compile_definitions(JOB_L3_PER_CCD_BYTES=16777216) # 16MB
    add_compile_definitions(JOB_DEFAULT_WS_MB=256)         # Safe default
elseif(JOB_L3_MB STREQUAL "8")
    add_compile_definitions(JOB_BLOCK_SIZE=64)
    add_compile_definitions(JOB_L3_PER_CCD_BYTES=8388608)   # 8MB
    add_compile_definitions(JOB_DEFAULT_WS_MB=128)          # Safe default
else()
    message(FATAL_ERROR "JOB_L3_MB must be 8, 16, or 32!")
endif()



option(JOB_TEST_BENCHMARKS "build and run the benchhmarks in the tests" ON)
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND JOB_TEST_BENCHMARKS)
    message("-- Building benchmarks into tests")
    add_compile_definitions(JOB_TEST_BENCHMARKS)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND JOB_TEST_BENCHMARKS)
    message("-- Invaild Build type for for benchmarks")
else()
    message("-- Not adding tests for benchmarks")
endif()



option(JOB_CUDA "Add nvidia cuda support " OFF)
option(JOB_QT_APPS "Build the Qt6 applications that are supported" OFF)
