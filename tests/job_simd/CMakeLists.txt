cmake_minimum_required(VERSION 3.16)

add_executable(test_job_simd
    main.cpp
    test_job_simd_utils.h
    test_job_simd_avx.cpp
    test_job_simd_math.cpp
    test_job_simd_for.cpp ## should get moved over to threads
    test_job_simd_transpose.cpp
)

target_compile_options(test_job_simd PRIVATE -freflection -fcontracts)
target_link_options(test_job_simd PRIVATE -fcontracts)

set_target_properties(test_job_simd PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(test_job_simd PRIVATE
    ${CatchTwo_LIBRARIES}
    atomic
    Threads::Threads
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
)

target_include_directories(test_job_simd PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_simd COMMAND test_job_simd)
