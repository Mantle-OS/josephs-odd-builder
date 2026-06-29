cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME "pkg")
set(DESC "A library for ledger in ai package manager")
set(VER 0.1)

project(${TARGET_NAME}
    VERSION ${VER}
    DESCRIPTION ${DESC}
    LANGUAGES CXX
)

# Notes
# include prefix:  "aipkg"
# c_struct prefix:  "AiPkg[ClassName]"
job_msgpack_gen_lib(
    NAME ${TARGET_NAME}
    VERSION ${VER}
    DESCRIPTION ${DESC}
    SCHEMA_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/input/pkg_file.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/pkg_package.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/pkg_depends.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/pkg_cache.yaml
    )
