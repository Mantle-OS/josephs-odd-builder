cmake_minimum_required(VERSION 3.16)

add_executable(test_job_simd
    main.cpp
    test_job_simd_utils.h
    test_job_simd_avx.cpp
    test_job_simd_math.cpp
    test_job_simd_for.cpp
    test_job_simd_transpose.cpp
)

if(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Threads PUBLIC atomic)
elseif(JOB_WINDOWS)
    target_link_libraries(JosephsOddBuilder_Threads PUBLIC ws2_32)
endif()

target_link_libraries(test_job_simd PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Simd
)

target_include_directories(test_job_simd PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_simd COMMAND test_job_simd)
