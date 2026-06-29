# SPDX-License-Identifer: MIT
include_guard(GLOBAL)
include(GNUInstallDirs)

function(job_msgpack_gen_lib)
    set(options BUILD_SHARED)
    set(one_val_args
        NAME
        VERSION
        DESCRIPTION
    )
    set(multi_value_args
        SCHEMA_FILES
        DEPENDENCIES
        TARGET_EXTRA_INCLUDE
    )
    cmake_parse_arguments(MSG_PACK_ARGS "${options}" "${one_val_args}" "${multi_value_args}" ${ARGN})

    if(NOT MSG_PACK_ARGS_NAME)
        message(FATAL_ERROR "NAME is required")
    endif()

    # find_package(Boost REQUIRED)
    cmake_policy(SET CMP0167 NEW) # crazy boost stuff
    find_package(msgpack-cxx REQUIRED)

    get_target_property(MSGPACK_CXX_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(BOOST_LIBS Boost::boost INTERFACE_LINK_LIBRARIES)

    set(PKG_EXTRA_CFLAGS "-I${MSGPACK_CXX_INCLUDE_DIRS} -I${Boost_INCLUDE_DIRS}")
    set(PKG_EXTRA_LIBS "${BOOST_LIBS}")

    set(MSG_PACK_LIB_NAME "${MSG_PACK_ARGS_NAME}_lib")

    if(NOT MSG_PACK_ARGS_SCHEMA_FILES)
        message(FATAL_ERROR "Need schema files to generate code....")
    endif()

    # Setup out directory based on current binary directory scope
    set(MSG_PACK_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set(OUT_SUBDIR "${MSG_PACK_OUT_DIR}/${MSG_PACK_ARGS_NAME}")
    file(MAKE_DIRECTORY ${OUT_SUBDIR})

    # make the files show up in IDE's with a safe unique suffix
    add_custom_target(${MSG_PACK_ARGS_NAME}_schema_files SOURCES ${MSG_PACK_ARGS_SCHEMA_FILES})

    set(GENERATED_SRCS)
    set(GENERATED_HDRS)

    foreach(IN_FILE ${MSG_PACK_ARGS_SCHEMA_FILES})
        get_filename_component(IN_WE ${IN_FILE} NAME_WE)

        set(GENERATED_SRC "${OUT_SUBDIR}/${IN_WE}.cpp")
        set(GENERATED_HDR "${OUT_SUBDIR}/${IN_WE}.hpp")

        list(APPEND GENERATED_SRCS ${GENERATED_SRC})
        list(APPEND GENERATED_HDRS ${GENERATED_HDR})

        add_custom_command(
            OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_SUBDIR}
            COMMAND "$<TARGET_FILE:job_msg_gen>"
            ARGS --schemas ${IN_FILE} --out ${OUT_SUBDIR}
            DEPENDS ${IN_FILE} job_msg_gen
            COMMENT "Generating code from ${IN_FILE}"
            VERBATIM
        )
    endforeach()

    # Determine Library Link State
    set(MSG_PACK_SHARED OFF)
    if(MSG_PACK_ARGS_BUILD_SHARED)
        set(MSG_PACK_SHARED ON)
    endif()

    if(MSG_PACK_SHARED)
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

    # Modern target includes using generator expressions
    # ${OUT_SUBDIR}
    target_include_directories(${MSG_PACK_LIB_NAME} PUBLIC
        $<BUILD_INTERFACE:${MSG_PACK_OUT_DIR}>
    )

    if(MSG_PACK_ARGS_TARGET_EXTRA_INCLUDE)
        target_include_directories(${MSG_PACK_LIB_NAME} PUBLIC ${MSG_PACK_ARGS_TARGET_EXTRA_INCLUDE})
    endif()

    # Compile-time dependencies
    set(MSG_PACK_DEPS
        Threads::Threads
        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost
        JosephsOddBuilder_Serializer
        JosephsOddBuilder_Serializer_MsgPack
    )

    if(MSG_PACK_ARGS_DEPENDENCIES)
        list(APPEND MSG_PACK_DEPS ${MSG_PACK_ARGS_DEPENDENCIES})
    endif()

    target_link_libraries(${MSG_PACK_LIB_NAME} PUBLIC ${MSG_PACK_DEPS})

    install(TARGETS ${MSG_PACK_LIB_NAME}
        EXPORT ${MSG_PACK_ARGS_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endfunction()