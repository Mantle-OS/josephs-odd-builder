cmake_minimum_required(VERSION 3.16)

option(JOB_QT_COMPARE "Link and bring in QtCore to test against sgnals and its meta system" OFF)
set(JOB_CORE_TEST_OBJECTS
    test_job_object_fixtures.h
    test_job_base_obj.cpp
    test_job_signal.cpp
    test_job_object.cpp
    test_job_light_object.cpp
    test_job_binary.cpp


    test_job_obj_grammar.cpp
    test_job_obj_members.cpp
    test_job_obj_class.cpp
    test_job_obj_ptr.cpp
    test_job_obj_json.cpp

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

add_executable(test_job_obj
    main.cpp
    ${JOB_CORE_TEST_OBJECTS}
)
## TODO remove me later
if(JOB_QT_COMPARE)
    target_link_libraries(test_job_obj PRIVATE Qt6::Core)
endif()

target_compile_options(test_job_obj PRIVATE -freflection -fcontracts)
target_link_options(test_job_obj PRIVATE -fcontracts)

set_target_properties(test_job_obj PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(test_job_obj PRIVATE
    ${CatchTwo_LIBRARIES}
    atomic
    Threads::Threads
    JosephsOddBuilder_Core
)

target_include_directories(test_job_obj PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_obj COMMAND test_job_obj)
