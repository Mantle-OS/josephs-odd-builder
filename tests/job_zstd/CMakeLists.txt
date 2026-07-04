cmake_minimum_required(VERSION 3.16)

add_executable(test_job_zstd
    transient_test_filesystem.h
    transient_test_filesystem.cpp
    transient_test_corruption.h
    transient_test_corruption.cpp
    main.cpp
    test_job_zstd_options.cpp
    test_job_zstd_dir_guard.cpp
    test_job_zstd_wire.cpp
    test_job_zstd_entry.cpp
    test_job_zstd_io.cpp
    test_job_zstd_compressor.cpp
    test_job_zstd_decompressor.cpp
    test_job_zstd_encrypting_transport.cpp
    test_job_zstd_decrypting_transport.cpp
    test_job_zstd_compressor_crypto.cpp
    test_job_zstd_decompressor_crypto.cpp
    test_job_zstd_sign.cpp
    test_job_zstd.cpp
)

target_link_libraries(test_job_zstd PRIVATE
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    ${LibSodium_LIBRARIES} JosephsOddBuilder_Crypto
    ${LibZstd_LIBRARIES} JosephsOddBuilder_Zstd
    ${CatchTwo_LIBRARIES}
)

target_include_directories(test_job_zstd PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_zstd COMMAND test_job_zstd)
