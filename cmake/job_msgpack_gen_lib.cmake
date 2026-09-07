# SPDX-License-Identifier: GPL-3.0-only

include_guard(GLOBAL)

include(GNUInstallDirs)
include(GenerateExportHeader)

function(job_msgpack_gen_lib)
    set(options
        BUILD_SHARED
    )

    set(one_val_args
        NAME
        VERSION
        DESCRIPTION
        EXPORT_HEADER
        EXPORT_MACRO
    )

    set(multi_value_args
        SCHEMA_FILES
        DEPENDENCIES
        TARGET_EXTRA_INCLUDE
    )

    cmake_parse_arguments(
        MSG_PACK_ARGS
        "${options}"
        "${one_val_args}"
        "${multi_value_args}"
        ${ARGN}
    )

    if(NOT MSG_PACK_ARGS_NAME)
        message(FATAL_ERROR "NAME is required")
    endif()

    if(NOT MSG_PACK_ARGS_SCHEMA_FILES)
        message(FATAL_ERROR "Need schema files to generate code....")
    endif()

    if(NOT MSG_PACK_ARGS_EXPORT_HEADER)
        message(FATAL_ERROR "EXPORT_HEADER is required")
    endif()

    if(NOT MSG_PACK_ARGS_EXPORT_MACRO)
        message(FATAL_ERROR "EXPORT_MACRO is required")
    endif()

    cmake_policy(SET CMP0167 NEW)

    find_package(Boost REQUIRED)
    find_package(msgpack-cxx REQUIRED)

    set(MSG_PACK_LIB_NAME "${MSG_PACK_ARGS_NAME}_lib")

    set(MSG_PACK_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set(OUT_SUBDIR "${MSG_PACK_OUT_DIR}/${MSG_PACK_ARGS_NAME}")

    file(MAKE_DIRECTORY "${OUT_SUBDIR}")

    add_custom_target(
        ${MSG_PACK_ARGS_NAME}_schema_files
        SOURCES ${MSG_PACK_ARGS_SCHEMA_FILES}
    )

    set(GENERATED_SRCS)
    set(GENERATED_HDRS)

    foreach(IN_FILE ${MSG_PACK_ARGS_SCHEMA_FILES})
        get_filename_component(IN_WE "${IN_FILE}" NAME_WE)

        set(GENERATED_SRC "${OUT_SUBDIR}/${IN_WE}.cpp")
        set(GENERATED_HDR "${OUT_SUBDIR}/${IN_WE}.hpp")

        list(APPEND GENERATED_SRCS "${GENERATED_SRC}")
        list(APPEND GENERATED_HDRS "${GENERATED_HDR}")

        add_custom_command(
            OUTPUT
                "${GENERATED_SRC}"
                "${GENERATED_HDR}"
            COMMAND
                ${CMAKE_COMMAND} -E make_directory "${OUT_SUBDIR}"
            COMMAND
                "$<TARGET_FILE:job_msg_gen>"
                --schemas "${IN_FILE}"
                --out "${OUT_SUBDIR}"
                --export-header "${MSG_PACK_ARGS_EXPORT_HEADER}"
                --export-macro "${MSG_PACK_ARGS_EXPORT_MACRO}"
            DEPENDS
                "${IN_FILE}"
                job_msg_gen
            COMMENT
                "Generating code from ${IN_FILE}"
            VERBATIM
        )
    endforeach()

    if(MSG_PACK_ARGS_BUILD_SHARED)
        add_library(${MSG_PACK_LIB_NAME} SHARED
            ${GENERATED_SRCS}
            ${GENERATED_HDRS}
        )
    else()
        add_library(${MSG_PACK_LIB_NAME} STATIC
            ${GENERATED_SRCS}
            ${GENERATED_HDRS}
        )
    endif()

    string(REGEX REPLACE "_EXPORT$" "" MSG_PACK_EXPORT_BASE "${MSG_PACK_ARGS_EXPORT_MACRO}")

    set(MSG_PACK_EXPORT "${OUT_SUBDIR}/${MSG_PACK_ARGS_EXPORT_HEADER}")
    generate_export_header(${MSG_PACK_LIB_NAME}
        BASE_NAME ${MSG_PACK_EXPORT_BASE}
        EXPORT_FILE_NAME "${MSG_PACK_EXPORT}"
    )

    set_target_properties(${MSG_PACK_LIB_NAME} PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF

        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )

    target_compile_options(${MSG_PACK_LIB_NAME}
        PRIVATE
            -freflection
            -fcontracts
    )

    target_include_directories(${MSG_PACK_LIB_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${MSG_PACK_OUT_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

            ## export file
            $<BUILD_INTERFACE:${OUT_SUBDIR}>
    )

    if(MSG_PACK_ARGS_TARGET_EXTRA_INCLUDE)
        target_include_directories(${MSG_PACK_LIB_NAME}
            PUBLIC
                ${MSG_PACK_ARGS_TARGET_EXTRA_INCLUDE}
        )
    endif()

    set(MSG_PACK_DEPS
        Threads::Threads
        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost

        JosephsOddBuilder_Core
        JosephsOddBuilder_IO

        JosephsOddBuilder_Serializer
        JosephsOddBuilder_Serializer_MsgPack
    )

    if(MSG_PACK_ARGS_DEPENDENCIES)
        list(APPEND MSG_PACK_DEPS ${MSG_PACK_ARGS_DEPENDENCIES})
    endif()

    target_link_libraries(${MSG_PACK_LIB_NAME}
        PUBLIC
            ${MSG_PACK_DEPS}
    )

    install(TARGETS ${MSG_PACK_LIB_NAME}
        EXPORT ${MSG_PACK_ARGS_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(FILES ${MSG_PACK_EXPORT} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MSG_PACK_ARGS_NAME})
    install(FILES ${GENERATED_HDRS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${MSG_PACK_ARGS_NAME})
endfunction()