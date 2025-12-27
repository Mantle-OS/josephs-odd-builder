cmake_minimum_required(VERSION 3.16)

add_executable(test_job_ai
    # ../test_spin_till.h
    main.cpp
    # test_activation_math.cpp
    # test_aligned_allocator.cpp
    # test_tensor_view.cpp
    # test_ai_shm_integration.cpp
    # test_unix_genome.cpp
    # test_gemm.cpp
    # test_flash_attention.cpp
    # test_mlp.cpp
    # test_attention_integration.cpp
    # test_fmm_adapter.cpp
    test_verlet_adapter.cpp
    # test_bh_adapter.cpp
    # test_stencil_adapter.cpp
    # test_rk4_adapter.cpp
    # test_moe.cpp
    # test_runner.cpp
    # test_xor.cpp
    # test_cartpole.cpp
    # test_text_bard.cpp # heavey

    # test_chat_bard.cpp
)

target_link_libraries(test_job_ai PRIVATE
    ${CatchTwo_LIBRARIES}
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Ansi
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Crypto
    JosephsOddBuilder_Net
    JosephsOddBuilder_Science
    JosephsOddBuilder_Ai
)
target_compile_options(JosephsOddBuilder_Ai PRIVATE ${JOB_CXX_FLAGS} )
target_include_directories(test_job_ai PRIVATE ${CatchTwo_INCLUDE_DIRS} )
# CHECKPOINT: v1
