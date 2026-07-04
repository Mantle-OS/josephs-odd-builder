cmake_minimum_required(VERSION 3.16)

add_executable(test_job_crypto
    transient_test_file.h
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
)

target_link_libraries(test_job_crypto PRIVATE
    Threads::Threads
    atomic
    ${LibSodium_LIBRARIES}
    JosephsOddBuilder_Crypto
    ${CatchTwo_LIBRARIES}
)

target_include_directories(test_job_crypto PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_crypto COMMAND test_job_crypto)
