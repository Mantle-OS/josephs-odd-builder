cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME "job_msg_gen")

project(${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION "use the message pack compiler too make a lib"
    LANGUAGES CXX
)

job_msgpack_gen_lib(
    NAME ${TARGET_NAME}
    VERSION 0.1
    DESCRIPTION "use the message pack compiler too make a lib"
    LANGUAGES CXX
    SCHEMA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_req.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_user.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/example_tx.json
)
