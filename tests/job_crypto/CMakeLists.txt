cmake_minimum_required(VERSION 3.16)

add_executable(test_job_crypto
    ../transient_test_file.h
    main.cpp
    test_job_crypto_init.cpp
    test_job_secure_mem.cpp
    test_job_random.cpp
    test_job_crypto_keys.cpp
    test_job_crypto_sign.cpp
    test_job_password_utils.cpp
    test_job_secret_box.cpp
    test_job_hash.cpp
    test_job_crypto.cpp
    test_job_x509_generator.cpp
    test_job_hmac_sha256.cpp
    test_job_sha.cpp
)

target_compile_options(test_job_crypto
    PRIVATE
      -freflection
      -fcontracts
)
target_link_options(test_job_crypto PRIVATE -fcontracts)

set_target_properties(test_job_crypto PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(test_job_crypto PRIVATE
    ${CatchTwo_LIBRARIES}
    atomic
    Threads::Threads
    ${LibSodium_LIBRARIES}
    ${LibCrypto_LIBRARIES}
    JosephsOddBuilder_Crypto
)

target_include_directories(test_job_crypto PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_crypto COMMAND test_job_crypto)
