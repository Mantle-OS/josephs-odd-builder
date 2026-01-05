cmake_minimum_required(VERSION 3.16)
add_executable(test_job_threads
    main.cpp
    # test_thread_science.h
    ../test_spin_till.h
    test_job_semaphore.cpp
    test_job_thread.cpp
    test_task_queue.cpp
    test_thread_pool.cpp
    test_thread_watcher.cpp
    test_async_event_loop.cpp
    test_job_io_async_thread.cpp
    test_parallels.cpp
    test_rr_sched.cpp
    test_ws_sched.cpp
    test_sporadic_sched.cpp
    test_actors.cpp
    test_graph.cpp
    test_job_pipeline.cpp
    test_bfs.cpp
    test_dijkstra.cpp
    test_branch_and_bound.cpp
)

target_link_libraries(test_job_threads PRIVATE
    Threads::Threads
    atomic
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Threads
)

target_include_directories(test_job_threads PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
# CHECKPOINT: v1.1
