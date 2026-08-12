cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilder
    VERSION 0.1
    LANGUAGES CXX
)

# Build time options and overrrides
include(cmake/build_options.cmake)

## Hardware checker and some compiler flags
include(cmake/check_hardware.cmake)

## cuda crap
include(cmake/check_nvidia.cmake)

## build time deps
include(cmake/build_deps.cmake)

## function for the massagepack generator
if(JOB_BUILD_SERIALIZER_MSGPACK)
    include(cmake/job_msgpack_gen_lib.cmake)
endif()

if(JOB_QT)
    ## Order matters here. stable diffusion cpp MUST come before Llama as stable diffusion cpp builds GGML which llama depends on
    include(cmake/cmake_stable_diffusion_cpp_build_flags.cmake)
    include(cmake/cmake_llama_cpp_build_flags.cmake)
endif()

add_subdirectory(libs)


# function(jobDumpCmakeVariables prefix)
#     get_cmake_property(allVariables VARIABLES)

#     list(FILTER allVariables INCLUDE REGEX "^${prefix}")
#     list(REMOVE_DUPLICATES allVariables)
#     list(SORT allVariables)

#     message(STATUS "")
#     message(STATUS "============================================================")
#     message(STATUS "${prefix} CMake Variables")
#     message(STATUS "============================================================")

#     foreach(variableName IN LISTS allVariables)
#         get_property(isCacheVariable CACHE "${variableName}" PROPERTY TYPE SET)

#         if(isCacheVariable)
#             get_property(cacheType CACHE "${variableName}" PROPERTY TYPE)
#             get_property(cacheValue CACHE "${variableName}" PROPERTY VALUE)

#             message(STATUS
#                 "${variableName} = ${${variableName}} "
#                 "[CACHE:${cacheType}=${cacheValue}]"
#             )
#         else()
#             message(STATUS "${variableName} = ${${variableName}}")
#         endif()
#     endforeach()

#     message(STATUS "============================================================")
#     message(STATUS "")
# endfunction()

# jobDumpCmakeVariables("GGML_")

# message(FATAL_ERROR "Intentional stop after pristine GGML configuration dump")




add_subdirectory(apps)

enable_testing()
add_subdirectory(tests)

add_subdirectory(docs)

add_custom_target(github_ci SOURCES .github/workflows/test.yml )

include(cmake/cmake_print_status.cmake)