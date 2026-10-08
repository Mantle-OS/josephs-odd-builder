cmake_minimum_required(VERSION 3.16)

set(JOB_CORE_TEST_CONTAINERS_FILES)
option(JOB_CORE_TEST_CONTAINERS "Test the job_core containers stack. Note this saturates the machine or trys to" OFF)
if(JOB_CORE_TEST_CONTAINERS)
    list(APPEND JOB_CORE_TEST_CONTAINERS_FILES
        test_job_list.cpp
        test_job_hash.cpp
        test_job_map.cpp
        test_job_obj_hash.cpp
    )
endif()

add_executable(test_job_core
    ../test_spin_till.h
    main.cpp
    test_job_siphash.cpp
    test_timer.cpp
    test_logger.cpp
    test_string_utils.cpp
    ${JOB_CORE_TEST_CONTAINERS_FILES}

)

target_compile_options(test_job_core PRIVATE -freflection -fcontracts)
target_link_options(test_job_core PRIVATE -fcontracts)
set_target_properties(test_job_core PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(test_job_core PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
)

target_include_directories(test_job_core PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_core COMMAND test_job_core)
