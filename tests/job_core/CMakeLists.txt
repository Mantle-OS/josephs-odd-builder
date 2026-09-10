cmake_minimum_required(VERSION 3.16)

option(JOB_QT_COMPARE "Link and bring in QtCore to test against sgnals and its meta system" OFF)

set(JOB_CORE_TEST_OBJECTS
    test_job_binary.cpp
    test_job_object_fixtures.h
    test_job_base_obj.cpp
    test_job_signal.cpp
    test_job_object.cpp
    test_job_light_object.cpp
)

## TODO remove me we beat this horse to death later Not now
if(JOB_QT_COMPARE)
    set(CMAKE_AUTOMOC ON)
    find_package(Qt6 6.2 COMPONENTS Core REQUIRED)
    list(APPEND JOB_CORE_TEST_OBJECTS
        q_test_base.h
        q_test_base.cpp
        test_q_object.cpp
    )
endif()


option(JOB_CORE_TEST_CONTAINERS "Test the job_core containers stack. Note this saturates the machine or trys to" OFF)
if(JOB_CORE_TEST_CONTAINERS)
    list(APPEND JOB_CORE_TEST_OBJECTS
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
    ${JOB_CORE_TEST_OBJECTS}
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

## TODO remove me later
if(JOB_QT_COMPARE)
    target_link_libraries(test_job_core PRIVATE Qt6::Core)
endif()

target_include_directories(test_job_core PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_core COMMAND test_job_core)
