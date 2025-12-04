cmake_minimum_required(VERSION 3.16)
add_compile_options(-mavx -mavx2 -mfma)

add_executable(test_job_ai
    ../test_spin_till.h
    main.cpp
    # test_activation_math.cpp
    # test_aligned_allocator.cpp
    # test_tensor_view.cpp
    # test_ai_shm_integration.cpp
    # test_unix_genome.cpp
    # test_gemm.cpp
    # test_flash_attention.cpp
    # test_mlp.cpp
    # test_coach.cpp
    test_moe.cpp
)

target_link_libraries(test_job_ai PRIVATE
    ${CatchTwo_LIBRARIES}
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Crypto
    JosephsOddBuilder_Net
    JosephsOddBuilder_Science
    JosephsOddBuilder_Ai
)
target_compile_options(JosephsOddBuilder_Ai PRIVATE
    -mavx -mavx2 -mfma
)
target_include_directories(test_job_ai PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
# CHECKPOINT: v1
