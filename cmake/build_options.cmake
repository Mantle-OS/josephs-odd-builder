include(CheckCXXCompilerFlag)
set(CMAKE_CXX_STANDARD 23) # thanks nvidia ..... So what we are going to do here later on and cheat ...... we will be turning this global off
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW) # for reproducible timestamps
endif()

##############################
# AVX Settings
##############################
option(JOB_AVX_FLAG           "build with avx" OFF)
option(JOB_AVX_TWO_FLAG       "build with avx two" OFF)
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
option(JOB_CUDA   "Add nvidia cuda support " ON)
option(JOB_USE_SYSTEM_GGML   "use system ggml" ON)
option(JOB_OPENGL "Add full openGl support " ON)
option(JOB_APPS   "Build the Example applications that are supported" ON)
option(JOB_TESTS  "Build the job tests" OFF)

##############################
# CICD and Benchmarks.
##############################
option(JOB_CI_BUILD "Enable settings specific to CI environments" OFF)
if(JOB_CI_BUILD)
    add_compile_definitions(JOB_CI_BUILD)
    add_compile_definitions(JOB_CI_MAX_THREADS=4)
endif()

if(JOB_TESTS)
    option(JOB_TEST_BENCHMARKS "build and run the benchhmarks in the tests (release mode only)" ${JOB_TESTS})
    if(CMAKE_BUILD_TYPE STREQUAL "Release" AND JOB_TEST_BENCHMARKS)
        message("-- Building benchmarks into tests")
        add_compile_definitions(JOB_TEST_BENCHMARKS)
    elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Release" AND JOB_TEST_BENCHMARKS)
        message("-- Invaild Build type for for benchmarks")
    else()
        message("-- Not adding tests for benchmarks")
    endif()
endif()

##############################
# Core Libraries
##############################
option(JOB_BUILD_CORE "Build the job_core library" ON)
option(JOB_BUILD_THREADS "Build the job_threads library" ON)

option(JOB_BUILD_CRYPTO "Build the job_crypto library" ON)
option(JOB_BUILD_IO "Build the job_io library" ON)
option(JOB_BUILD_NET "Build the job_net library" ON)

option(JOB_BUILD_SOUND "Build the job_sound library" ON)
option(JOB_BUILD_UART "Build the job_uart library" ON)
option(JOB_BUILD_USB "Build the job_usb library" ON)

option(JOB_BUILD_INPUT "Build the job_input library" ON)
option(JOB_BUILD_INPUT_EVDEV "Build the job_input wth libevdev library" ON)
option(JOB_BUILD_INPUT_LIBINPUT "Build the job_input wth libinput library" ON)

option(JOB_BUILD_RASTER "Build the job_raster library" ON)
option(JOB_BUILD_OPENGL "Build the job_opengl library" ON)
option(JOB_BUILD_VULKAN "Build the job_vulkan library" ON)

option(JOB_BUILD_RENDER "Build the job_render library" ON)
option(JOB_BUILD_RENDER_FB "Build the Linux framebuffer backend for job_render" ON)
option(JOB_BUILD_RENDER_KMS "Build the KMS/DRM backend for job_render" ON)

option(JOB_BUILD_RENDER_WAYLAND "Build the Wayland backend for job_render" ON)
option(JOB_BUILD_RENDER_WAYLAND_CLIENT "Build the Wayland client for job_render" ON)
option(JOB_BUILD_RENDER_WAYLAND_SERVER "Build the Wayland server for job_render" ON)


# option(JOB_BUILD_RENDER_MIR "Build the KMS MIR backend for job_render" OFF)

option(JOB_BUILD_GUI "Build the job_gui library" ON)
# option(JOB_BUILD_DAMAGE "Build the job_damage library" OFF)
option(JOB_BUILD_TUI "Build the job_tui library" ON)

option(JOB_BUILD_ZSTD "Build the job_zstd library" ON)
option(JOB_BUILD_SCIENCE "Build the job_science library" ON)

option(JOB_BUILD_ANSI "Build the job_ansi library" OFF)
option(JOB_BUILD_ANSI_LEGACY "Build the job_ansi library" ON)
option(JOB_BUILD_TUI "Build the job_tui library" OFF)

option(JOB_BUILD_AI "Build the micro job_ai library" OFF)
option(JOB_BUILD_GGML "Build JobGgml " ON)
option(JOB_BUILD_TOKEN "Build the job_token library" ON)
option(JOB_BUILD_MODEL "Build the job_model library" ON)
option(JOB_BUILD_AI_TOOLS "Build the job_ai_tools library" ON)

option(JOB_BUILD_SERIALIZER "Build the job_serializer library" ON)
    option(JOB_BUILD_SERIALIZER_MSGPACK "Enable MsgPack backend for Job Serlizer" ON)
        option(JOB_BUILD_AIPKG_SCHEMA "Build the generated AiPkg schema library" ${JOB_BUILD_SERIALIZER_MSGPACK})
        option(JOB_BUILD_AIPKG "Build the job_aipkg package library" ${JOB_BUILD_SERIALIZER_MSGPACK})
    option(JOB_BUILD_SERIALIZER_FLATBUFFERS "Enable FlatBuffers backend" OFF)
