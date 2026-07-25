## LLAMA.CPP
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
    ## this was fun lol
    set(LLGUIDANCE_SRC "${CMAKE_CURRENT_BINARY_DIR}/llguidance/source" CACHE INTERNAL "Forced Submodule Path")

    string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
    if (_build_type_lower STREQUAL "debug")
        set(LLGUIDING_CONFIG_DIR "debug")
        set(CARGO_BUILD_FLAGS "")
        message(STATUS "[LLGuidance] Dynamically configured for Cargo local development debug bounds.")
    else()
        set(LLGUIDING_CONFIG_DIR "release")
        set(CARGO_BUILD_FLAGS "--release")
    endif()


    # Cargo tracks CARGO_BUILD_FLAGS in Debug, it will build into target/debug. ...
    set(LLGUIDANCE_PATH "${LLGUIDANCE_SRC}/target/${LLGUIDING_CONFIG_DIR}" CACHE INTERNAL "Forced Submodule Path")

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

    # True include tracking target
    include_directories(SYSTEM BEFORE "${LLGUIDANCE_SRC}/parser" )

    set(LLGUIDANCE_STATIC_ARCHIVE "${LLGUIDANCE_PATH}/${LLGUIDANCE_LIB_NAME}")
endif()

add_subdirectory(3rdparty/llama.cpp)
