cmake_minimum_required(VERSION 3.16)
set(TARGET_NAME "aipkg_ledger")
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
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_attestation.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_audit_node.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_audit_path.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_block.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_consistency.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_delegate.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_devlink.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_devproof.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_devsig.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_hash32.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_revoke.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_reward.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_sth.yaml
        ${CMAKE_CURRENT_SOURCE_DIR}/input/ledger_tx.yaml
)


