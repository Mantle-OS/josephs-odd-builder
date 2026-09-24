# SPDX-License-Identifier: GPL-3.0-only

include_guard(GLOBAL)

function(job_wayland_scanner_generate OUTPUT_VARIABLE)
    set(one_value_args
        MODE
        OUTPUT_SUFFIX
        OUTPUT_EXTENSION
        OUTPUT_DIR
    )

    set(multi_value_args
        SCHEMA_FILES
    )

    cmake_parse_arguments(
        WAY_GEN_ARGS
        ""
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
    )

    if(NOT WAY_GEN_ARGS_MODE)
        message(FATAL_ERROR "MODE is required")
    endif()

    if(NOT WAY_GEN_ARGS_OUTPUT_SUFFIX)
        message(FATAL_ERROR "OUTPUT_SUFFIX is required")
    endif()

    if(NOT WAY_GEN_ARGS_OUTPUT_EXTENSION)
        message(FATAL_ERROR "OUTPUT_EXTENSION is required")
    endif()

    if(NOT WAY_GEN_ARGS_OUTPUT_DIR)
        message(FATAL_ERROR "OUTPUT_DIR is required")
    endif()

    set(GENERATED_FILES)

    foreach(IN_FILE IN LISTS WAY_GEN_ARGS_SCHEMA_FILES)
        get_filename_component(IN_WE "${IN_FILE}" NAME_WE)

        set(OUT_FILE
            "${WAY_GEN_ARGS_OUTPUT_DIR}/${IN_WE}-${WAY_GEN_ARGS_OUTPUT_SUFFIX}.${WAY_GEN_ARGS_OUTPUT_EXTENSION}"
        )

        add_custom_command(
            OUTPUT
                "${OUT_FILE}"
            COMMAND
                ${CMAKE_COMMAND} -E make_directory "${WAY_GEN_ARGS_OUTPUT_DIR}"
            COMMAND
                "${JOB_WAYLAND_SCANNER}"
                --strict
                "${WAY_GEN_ARGS_MODE}"
                "${IN_FILE}"
                "${OUT_FILE}"
            DEPENDS
                "${IN_FILE}"
            COMMENT
                "Generating Wayland ${WAY_GEN_ARGS_MODE} from ${IN_FILE}"
            VERBATIM
        )

        list(APPEND GENERATED_FILES "${OUT_FILE}")
    endforeach()

    set(${OUTPUT_VARIABLE} "${GENERATED_FILES}" PARENT_SCOPE)
endfunction()


function(job_wayland_scanner_lib)
    set(options
        BUILD_SHARED
    )

    set(one_value_args
        NAME
        VERSION
        DESCRIPTION
    )

    set(multi_value_args
        SERVER_SCHEMA_FILES
        CLIENT_SCHEMA_FILES
        ENUM_SCHEMA_FILES
        PRIVATE_SCHEMA_FILES
        PUBLIC_SCHEMA_FILES
        HEADERS
        SOURCES
        DEPENDENCIES
        TARGET_EXTRA_INCLUDE
    )

    cmake_parse_arguments(
        WAY_SCAN_ARGS
        "${options}"
        "${one_value_args}"
        "${multi_value_args}"
        ${ARGN}
    )

    if(NOT WAY_SCAN_ARGS_NAME)
        message(FATAL_ERROR "NAME is required")
    endif()

    if(NOT WAY_SCAN_ARGS_SERVER_SCHEMA_FILES)
        message(STATUS "${WAY_SCAN_ARGS_NAME}: no server schema files")
    endif()

    if(NOT WAY_SCAN_ARGS_CLIENT_SCHEMA_FILES)
        message(STATUS "${WAY_SCAN_ARGS_NAME}: no client schema files")
    endif()

    if(NOT WAY_SCAN_ARGS_ENUM_SCHEMA_FILES)
        message(STATUS "${WAY_SCAN_ARGS_NAME}: no enum schema files")
    endif()

    if(NOT WAY_SCAN_ARGS_PRIVATE_SCHEMA_FILES)
        message(STATUS "${WAY_SCAN_ARGS_NAME}: no private-code schema files")
    endif()

    if(NOT WAY_SCAN_ARGS_PUBLIC_SCHEMA_FILES)
        message(STATUS "${WAY_SCAN_ARGS_NAME}: no public-code schema files")
    endif()

    if(NOT WAY_SCAN_ARGS_SERVER_SCHEMA_FILES
       AND NOT WAY_SCAN_ARGS_CLIENT_SCHEMA_FILES
       AND NOT WAY_SCAN_ARGS_ENUM_SCHEMA_FILES
       AND NOT WAY_SCAN_ARGS_PRIVATE_SCHEMA_FILES
       AND NOT WAY_SCAN_ARGS_PUBLIC_SCHEMA_FILES)
        message(FATAL_ERROR "${WAY_SCAN_ARGS_NAME}: at least one schema file is required")
    endif()

    set(WAY_SCAN_LIB_NAME "${WAY_SCAN_ARGS_NAME}_lib")

    set(WAY_SCAN_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set(OUT_SUBDIR "${WAY_SCAN_OUT_DIR}/${WAY_SCAN_ARGS_NAME}")

    file(MAKE_DIRECTORY "${OUT_SUBDIR}")

    set(ALL_SCHEMA_FILES
        ${WAY_SCAN_ARGS_SERVER_SCHEMA_FILES}
        ${WAY_SCAN_ARGS_CLIENT_SCHEMA_FILES}
        ${WAY_SCAN_ARGS_ENUM_SCHEMA_FILES}
        ${WAY_SCAN_ARGS_PRIVATE_SCHEMA_FILES}
        ${WAY_SCAN_ARGS_PUBLIC_SCHEMA_FILES}
    )

    list(REMOVE_DUPLICATES ALL_SCHEMA_FILES)

    add_custom_target(${WAY_SCAN_ARGS_NAME}_schema_files SOURCES
        ${ALL_SCHEMA_FILES}
    )

    set(GENERATED_SRCS)
    set(GENERATED_HDRS)

    job_wayland_scanner_generate(
        CLIENT_GENERATED_HDRS
        MODE client-header
        OUTPUT_SUFFIX client-protocol
        OUTPUT_EXTENSION h
        OUTPUT_DIR "${OUT_SUBDIR}"
        SCHEMA_FILES ${WAY_SCAN_ARGS_CLIENT_SCHEMA_FILES}
    )

    job_wayland_scanner_generate(
        SERVER_GENERATED_HDRS
        MODE server-header
        OUTPUT_SUFFIX server-protocol
        OUTPUT_EXTENSION h
        OUTPUT_DIR "${OUT_SUBDIR}"
        SCHEMA_FILES ${WAY_SCAN_ARGS_SERVER_SCHEMA_FILES}
    )

    job_wayland_scanner_generate(
        ENUM_GENERATED_HDRS
        MODE enum-header
        OUTPUT_SUFFIX enum-protocol
        OUTPUT_EXTENSION h
        OUTPUT_DIR "${OUT_SUBDIR}"
        SCHEMA_FILES ${WAY_SCAN_ARGS_ENUM_SCHEMA_FILES}
    )

    job_wayland_scanner_generate(
        PRIVATE_GENERATED_SRCS
        MODE private-code
        OUTPUT_SUFFIX private-protocol
        OUTPUT_EXTENSION c
        OUTPUT_DIR "${OUT_SUBDIR}"
        SCHEMA_FILES ${WAY_SCAN_ARGS_PRIVATE_SCHEMA_FILES}
    )

    job_wayland_scanner_generate(
        PUBLIC_GENERATED_SRCS
        MODE public-code
        OUTPUT_SUFFIX public-protocol
        OUTPUT_EXTENSION c
        OUTPUT_DIR "${OUT_SUBDIR}"
        SCHEMA_FILES ${WAY_SCAN_ARGS_PUBLIC_SCHEMA_FILES}
    )

    list(APPEND GENERATED_HDRS
        ${CLIENT_GENERATED_HDRS}
        ${SERVER_GENERATED_HDRS}
        ${ENUM_GENERATED_HDRS}
    )

    list(APPEND GENERATED_SRCS
        ${PRIVATE_GENERATED_SRCS}
        ${PUBLIC_GENERATED_SRCS}
    )

    set_source_files_properties("${GENERATED_SRCS}" "${GENERATED_SRCS}" PROPERTIES GENERATED TRUE)

    if(WAY_SCAN_ARGS_BUILD_SHARED)
        add_library(${WAY_SCAN_LIB_NAME} SHARED
            ${GENERATED_SRCS}           ${GENERATED_HDRS}
            ${WAY_SCAN_ARGS_HEADERS}    ${WAY_SCAN_ARGS_SOURCES}
        )
    else()
        add_library(${WAY_SCAN_LIB_NAME} STATIC
            ${GENERATED_SRCS}           ${GENERATED_HDRS}
            ${WAY_SCAN_ARGS_HEADERS}    ${WAY_SCAN_ARGS_SOURCES}
        )
    endif()


    # add_custom_target(${WAY_SCAN_ARGS_NAME}_generated
    #     DEPENDS ${WAY_SCAN_LIB_NAME}
    #     SOURCES ${GENERATED_SRCS} ${GENERATED_HDRS}
    # )

    add_dependencies(
        ${WAY_SCAN_LIB_NAME}
        ${WAY_SCAN_ARGS_NAME}_schema_files
        # ${WAY_SCAN_ARGS_NAME}_generated
    )
    # message(FATAL_ERROR "GLASSES LOOK "
    #     "${GENERATED_SRCS} \n"
    #     "${GENERATED_HDRS} \n" )


    set_target_properties(${WAY_SCAN_LIB_NAME} PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    target_compile_options(${WAY_SCAN_LIB_NAME}
        PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-freflection>
            $<$<COMPILE_LANGUAGE:CXX>:-fcontracts>
    )

    target_link_options(${WAY_SCAN_LIB_NAME}
        PRIVATE
            $<$<LINK_LANGUAGE:CXX>:-fcontracts>
    )

    target_include_directories(${WAY_SCAN_LIB_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${WAY_SCAN_OUT_DIR}>
            $<BUILD_INTERFACE:${OUT_SUBDIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    if(WAY_SCAN_ARGS_TARGET_EXTRA_INCLUDE)
        target_include_directories(${WAY_SCAN_LIB_NAME} PUBLIC ${WAY_SCAN_ARGS_TARGET_EXTRA_INCLUDE})
    endif()

    set(WAY_SCAN_DEPS)

    if(WAY_SCAN_ARGS_DEPENDENCIES)
        list(APPEND WAY_SCAN_DEPS ${WAY_SCAN_ARGS_DEPENDENCIES})
    endif()

    if(WAY_SCAN_DEPS)
        target_link_libraries(${WAY_SCAN_LIB_NAME} PUBLIC ${WAY_SCAN_DEPS})
    endif()

    install(
        TARGETS ${WAY_SCAN_LIB_NAME}
        EXPORT ${WAY_SCAN_ARGS_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    if(GENERATED_HDRS)
        install(FILES ${GENERATED_HDRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${WAY_SCAN_ARGS_NAME})
    endif()

    if(WAY_SCAN_ARGS_HEADERS)
        install(FILES ${WAY_SCAN_ARGS_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${WAY_SCAN_ARGS_NAME})
    endif()
endfunction()