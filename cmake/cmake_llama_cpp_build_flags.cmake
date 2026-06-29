##
## Build time flags for building llama.cpp
set(LLAMA_USE_SYSTEM_GGML OFF)
set(LLAMA_WASM_MEM64 OFF)
set(BUILD_SHARED_LIBS OFF)
set(LLAMA_SANITIZE_THREAD    OFF)
set(LLAMA_SANITIZE_ADDRESS   OFF)
set(LLAMA_SANITIZE_UNDEFINED OFF)

# utils
set(LLAMA_BUILD_COMMON ON)

# extra artifacts
set(LLAMA_BUILD_TESTS     OFF)
set(LLAMA_BUILD_TOOLS     OFF)
set(LLAMA_BUILD_EXAMPLES  OFF)
set(LLAMA_BUILD_SERVER    ON)
set(LLAMA_BUILD_APP       OFF)
set(LLAMA_BUILD_UI        OFF)
set(LLAMA_USE_PREBUILT_UI OFF)

set(LLAMA_TOOLS_INSTALL OFF)
set(LLAMA_TESTS_INSTALL OFF)

# 3rd party
set(LLAMA_OPENSSL ON)
set(LLAMA_LLGUIDANCE  ON)
if (LLAMA_LLGUIDANCE)
    find_program(CARGO_EXECUTABLE cargo REQUIRED)

    set(LLGUIDANCE_SRC "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/llama.cpp/llguidance/source" CACHE INTERNAL "Forced Submodule Path")
    set(LLGUIDANCE_PATH "${LLGUIDANCE_SRC}/target/release" CACHE INTERNAL "Forced Submodule Path")

    if (WIN32)
        set(LLGUIDANCE_LIB_NAME "llguidance.lib" CACHE INTERNAL "")
    else()
        set(LLGUIDANCE_LIB_NAME "libllguidance.a" CACHE INTERNAL "")
    endif()

    # THE TRIPLE LOCK lol : Force Ninja to trace the rule chain through llama's core components
    if (TARGET llama-common AND TARGET llguidance_ext)
        add_dependencies(llama-common llguidance_ext)
    endif()

    if (TARGET llama AND TARGET llguidance_ext)
        add_dependencies(llama llguidance_ext)
    endif()

    if (TARGET ggml AND TARGET llguidance_ext)
        add_dependencies(ggml llguidance_ext)
    endif()

    # Clean up include tracking loops
    include_directories(SYSTEM BEFORE
        "${LLGUIDANCE_SRC}/libs/llguidance/include"
        "${LLGUIDANCE_SRC}/parser"
    )

    set(LLGUIDANCE_STATIC_ARCHIVE "${LLGUIDANCE_PATH}/${LLGUIDANCE_LIB_NAME}")
endif()
