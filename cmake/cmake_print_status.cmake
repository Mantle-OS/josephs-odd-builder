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

