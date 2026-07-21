cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0167 NEW) # crazy boost stuff
find_package(msgpack-cxx REQUIRED)
get_target_property(MSGPACK_CXX_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BOOST_LIBS Boost::boost INTERFACE_LINK_LIBRARIES)

add_executable(test_job_aipkg
    main.cpp
    test_job_aipkg_consistency.cpp
    test_job_aipkg_chain.cpp
    test_job_aipkg_sign.cpp
    test_job_aipkg_revocation_index.cpp
    test_job_aipkg_manifest.cpp
)

target_link_libraries(test_job_aipkg
    PRIVATE
        ${CatchTwo_LIBRARIES}
        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost
        JosephsOddBuilder_Core
        ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto
        ${LibZstd_LIBRARIES} JosephsOddBuilder_Zstd
        aipkg_ledger_lib pkg_lib JosephsOddBuilder_AiPkg
)

target_include_directories(test_job_aipkg PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_aipkg COMMAND test_job_aipkg)
