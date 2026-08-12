cmake_minimum_required(VERSION 3.16)

add_executable(test_job_core
    ../test_spin_till.h
    main.cpp
    test_timer.cpp
    test_logger.cpp
    test_job_siphash.cpp
    test_job_list.cpp
    test_job_hash.cpp
    test_job_map.cpp
    test_job_obj_hash.cpp
)

target_link_libraries(test_job_core PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
)

target_include_directories(test_job_core PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_core COMMAND test_job_core)
