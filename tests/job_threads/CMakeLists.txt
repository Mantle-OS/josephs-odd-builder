cmake_minimum_required(VERSION 3.16)

add_executable(test_job_threads
    main.cpp
    test_task_queue.cpp
    test_thread_pool.cpp
    test_async_event_loop.cpp
)

target_link_libraries(test_job_threads PRIVATE
    JosephsOddBuilder_Threads
    ${CatchTwo_LIBRARIES}
    Threads::Threads
)

target_include_directories(test_job_threads PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
