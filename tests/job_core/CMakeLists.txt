cmake_minimum_required(VERSION 3.16)

add_executable(test_job_core
    main.cpp
    test_timer.cpp
    test_logger.cpp
)

target_link_libraries(test_job_core PRIVATE
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Core
)

target_include_directories(test_job_core PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
