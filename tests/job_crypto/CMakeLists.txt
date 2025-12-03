cmake_minimum_required(VERSION 3.16)

add_executable(test_job_crypto
    main.cpp
    test_job_crypto_init.cpp
    test_job_secure_mem.cpp
    test_job_random.cpp
)

target_link_libraries(test_job_crypto PRIVATE
    Threads::Threads
    atomic
    ${LibSodium_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Crypto
    ${CatchTwo_LIBRARIES}
)

target_include_directories(test_job_crypto PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
# CHECKPOINT: v1.0
