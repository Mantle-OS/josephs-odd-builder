cmake_minimum_required(VERSION 3.16)



set(JOB_CORE_TEST_OBJECTS
    test_job_object_fixtures.h
    test_job_base_obj.cpp
    test_job_signal.cpp
    test_job_object.cpp
)

if(JOB_QT)
    set(CMAKE_AUTOMOC ON)
    list(APPEND JOB_CORE_TEST_OBJECTS
        q_test_base.h
        q_test_base.cpp
        test_q_object.cpp
    )
endif()


add_executable(test_job_core
    ../test_spin_till.h
    main.cpp
    # test_timer.cpp
    # test_logger.cpp
    # test_job_siphash.cpp
    # test_job_list.cpp
    # test_job_hash.cpp
    # test_job_map.cpp
    # test_job_obj_hash.cpp

    ${JOB_CORE_TEST_OBJECTS}

)
target_compile_options(test_job_core PRIVATE
    -freflection
    -fcontracts)

target_link_options(test_job_core PRIVATE
    -fcontracts
)

set_target_properties(test_job_core PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(test_job_core PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
)


if(JOB_QT)
    target_link_libraries(test_job_core PRIVATE
        Qt6::Core
    )
endif()

target_include_directories(test_job_core PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_core COMMAND test_job_core)
