cmake_minimum_required(VERSION 3.16)

add_executable(test_job_ai
    ../test_spin_till.h
    main.cpp
    test_activation_math.cpp
    test_aligned_allocator.cpp
    test_tensor_view.cpp
    test_ai_shm_integration.cpp
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
    JosephsOddBuilder_Ai
)

target_include_directories(test_job_ai PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
# CHECKPOINT: v1
