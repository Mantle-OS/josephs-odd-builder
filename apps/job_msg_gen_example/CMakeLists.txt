cmake_minimum_required(VERSION 3.16)
## INFO this will all be moved over to a cmake module once stable
project(job_msg_gen_lib
    VERSION 0.1
    DESCRIPTION "use the message pack compiler too make a lib"
    LANGUAGES CXX
)


cmake_policy(SET CMP0167 NEW) # crazy boost stuff
find_package(msgpack-cxx REQUIRED)

get_target_property(MSGPACK_CXX_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BOOST_LIBS Boost::boost INTERFACE_LINK_LIBRARIES)

set(GENERATOR_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY ${GENERATOR_OUT_DIR}/)

set(INPUT_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/input/example_req.yaml
    ${CMAKE_CURRENT_SOURCE_DIR}/input/example_user.yaml
    ${CMAKE_CURRENT_SOURCE_DIR}/input/example_tx.json
)

# make the files show up in IDE's
add_custom_target(job_msg_gen_example_files SOURCES
    ${INPUT_FILES}
)

set(GENERATED_SRCS)
set(GENERATED_HDRS)

# add_dependencies(job_msg_gen_lib job_msg_gen)
foreach(IN_FILE ${INPUT_FILES})
    get_filename_component(IN_WE ${IN_FILE} NAME_WE)

    ## this part .... stu[id hardcoded crazy
    set(GENERATED_SRC "${GENERATOR_OUT_DIR}/job_example/${IN_WE}.cpp")
    set(GENERATED_HDR "${GENERATOR_OUT_DIR}/job_example/${IN_WE}.hpp")

    list(APPEND GENERATED_SRCS ${GENERATED_SRC})
    list(APPEND GENERATED_HDRS ${GENERATED_HDR})

    add_custom_command(
        OUTPUT ${GENERATED_SRC} ${GENERATED_HDR}
        COMMAND "$<TARGET_FILE:job_msg_gen>"
        ARGS  --schemas ${IN_FILE}
          --out "${GENERATOR_OUT_DIR}/job_example"
        DEPENDS ${IN_FILE} job_msg_gen
        COMMENT "Generating code from ${IN_FILE}"
        VERBATIM
    )
endforeach()

add_library(job_msg_gen_lib SHARED
     ${GENERATED_SRCS}
     ${GENERATED_HDR}
)

target_include_directories(job_msg_gen_lib
    PUBLIC ${GENERATOR_OUT_DIR}
)

target_link_libraries(job_msg_gen_lib
    PRIVATE
        Threads::Threads
        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
        JosephsOddBuilder_Serializer
        JosephsOddBuilder_Serializer_MsgPack
)

install(TARGETS job_msg_gen_lib
    EXPORT job_msg_gen_example
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# CHECKPOINT: v1
