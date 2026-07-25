include(CheckCXXCompilerFlag)
set(CMAKE_CXX_STANDARD 20) # thanks nvidia .....
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## I can not get qt to shut up in the creator ... whatever
set(CMAKE_QT_CREATOR_ENABLE_MAINTENANCE_TOOL_PROVIDER OFF)
set(CMAKE_QT_CREATOR_ENABLE_PACKAGE_MANAGER_SETUP OFF)

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
# CPU Options
##############################
set(JOB_L1_KB "32" CACHE STRING "L1 data cache size per core in Kilobytes")
if(NOT JOB_L1_KB MATCHES "^(32|48|64)$")
    message(FATAL_ERROR "JOB_L1_KB must be 32, 48, or 64! You provided: ${JOB_L1_KB}")
endif()

set(JOB_L2_KB "512" CACHE STRING "L2 cache size per core in Kilobytes")
if(NOT JOB_L2_KB MATCHES "^(256|512|1024|2048)$")
    message(FATAL_ERROR "JOB_L2_KB must be 256, 512, 1024, or 2048! You provided: ${JOB_L2_KB}")
endif()

set(JOB_L3_MB "32" CACHE STRING "L3 Cache size per CCD in Megabytes")
if(NOT JOB_L3_MB MATCHES "^(32|16|8)$")
    message(FATAL_ERROR "JOB_L3_MB must be 8, 16, or 32! You provided: ${JOB_L3_MB}")
endif()


##############################
# Gate Keepers
##############################
option(JOB_CUDA "Add nvidia cuda support " ON)
option(JOB_QT   "Build the Qt6 applications that are supported" ON)


##############################
# CICD and Benchmarks.
##############################
option(JOB_CI_BUILD "Enable settings specific to CI environments" OFF)
if(JOB_CI_BUILD)
    add_compile_definitions(JOB_CI_BUILD)
endif()

option(JOB_TEST_BENCHMARKS "build and run the benchhmarks in the tests (release mode only)" ON)
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND JOB_TEST_BENCHMARKS)
    message("-- Building benchmarks into tests")
    add_compile_definitions(JOB_TEST_BENCHMARKS)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND JOB_TEST_BENCHMARKS)
    message("-- Invaild Build type for for benchmarks")
else()
    message("-- Not adding tests for benchmarks")
endif()


##############################
# Core Libraries
##############################

option(JOB_BUILD_CORE "Build the job_core library" ON)
option(JOB_BUILD_THREADS "Build the job_threads library" ON)
option(JOB_BUILD_CRYPTO "Build the job_crypto library" ON)
option(JOB_BUILD_ZSTD "Build the job_zstd library" ON)
option(JOB_BUILD_SIMD "Build the job_simd library" ON)
option(JOB_BUILD_CUDA "Build the job_cuda library" ON)
option(JOB_BUILD_IO "Build the job_io library" ON)
option(JOB_BUILD_UART "Build the job_uart library" ON)
option(JOB_BUILD_NET "Build the job_net library" ON)

option(JOB_BUILD_SERIALIZER "Build the job_serializer library" ON)
option(JOB_BUILD_SERIALIZER_MSGPACK "Enable MsgPack backend for Job Serlizer" ON)
option(JOB_BUILD_SERIALIZER_FLATBUFFERS "Enable FlatBuffers backend" OFF)

option(JOB_BUILD_SCIENCE "Build the job_science library" ON)
option(JOB_BUILD_AI "Build the job_ai library" ON)
option(JOB_BUILD_ANSI "Build the job_ansi library" ON)
option(JOB_BUILD_TUI "Build the job_tui library" ON)

##############################
# Qt Adapter Libraries
##############################

option(JOB_BUILD_QTAI "Build the Qt AI adapter libraries" ON)
option(JOB_BUILD_AIPKG_SCHEMA "Build the generated AiPkg schema library" ON)
option(JOB_BUILD_AIPKG "Build the job_aipkg package library" ON)
option(JOB_BUILD_QAIUTILS "Build the qaiutils Qt adapter library" ON)
option(JOB_BUILD_QSODIUM "Build the qsodium Qt adapter library" ON)
option(JOB_BUILD_QZSTD "Build the qzstd Qt adapter library" ON)
option(JOB_BUILD_QSD "Build the qsd diffusion adapter library" OFF)
option(JOB_BUILD_QLLAMA "Build the qllama inference adapter library" OFF)
option(JOB_BUILD_QHF "Build the qhf Hugging Face adapter library" OFF)
option(JOB_BUILD_QSESSIONMANAGER "Build the Qt session manager" OFF)

