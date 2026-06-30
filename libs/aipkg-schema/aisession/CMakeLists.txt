cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME "aisession")
set(DESC "A library for ledger in ai package manager")
set(VER 0.1)

project(${TARGET_NAME}
    VERSION ${VER}
    DESCRIPTION ${DESC}
    LANGUAGES CXX
)

# Notes
# include prefix:  "aisession"
# c_struct prefix:  "AiSession[ClassName]"
job_msgpack_gen_lib(
    NAME ${TARGET_NAME}
    VERSION ${VER}
    DESCRIPTION ${DESC}
    SCHEMA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/input/session_user.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/session_user_index.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/session_key.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/session_credential.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/session_provider.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/session_vault.yaml
    )
