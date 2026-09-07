cmake_minimum_required(VERSION 3.16)

add_executable(test_job_schema
    main.cpp
    test_job_schema_concepts.cpp
)

set_target_properties(test_job_schema PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(test_job_schema PRIVATE -freflection -fcontracts)
target_link_options(test_job_schema    PRIVATE -fcontracts)

target_link_libraries(test_job_schema PRIVATE
    Threads::Threads
    ${CatchTwo_LIBRARIES}
    ${YAMLCPP_LIBRARIES}
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Schema
)

target_include_directories(test_job_schema PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_schema COMMAND test_job_schema)