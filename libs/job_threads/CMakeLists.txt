cmake_minimum_required(VERSION 3.23)
project(JosephsOddBuilderThreads
    VERSION 0.1
    LANGUAGES CXX
)

# CORE
set(JOBTHREADS_HEADERS
    job_thread.h
    job_thread_pool.h
    job_thread_watcher.h
    job_semaphore.h
)

set(JOBTHREADS_SOURCES
    job_thread_pool.cpp
    job_thread_watcher.cpp
    job_thread.cpp
    job_semaphore.cpp
)

# UTILS
set(JOBTHREADS_UTILS_HEADERS
    utils/job_thread_options.h
    utils/job_thread_args.h
    utils/job_thread_metrics.h

    utils/job_latch.h

    utils/job_parallel_for.h
    utils/job_parallel_reduce.h

    utils/job_thread_node.h
    utils/job_thread_graph.h
    utils/job_thread_actor.h

    utils/job_pipeline_sink.h
    utils/job_pipeline_stage.h
    utils/job_pipeline.h

    utils/job_parallel_bfs.h
    utils/job_parallel_dijkstra.h

    utils/job_branch_and_bound.h

    # utils/job_monte_carlo_concept.h
    utils/job_monte_carlo.h
    utils/simd_for.h
)
set(JOBTHREADS_UTILS_SOURCES
    utils/job_thread_graph.cpp
)

# QUEUE's
set(JOBTHREADS_QUEUE_HEADERS
    queue/job_task_queue.h
    queue/job_mcmp_queue.h
)
set(JOBTHREADS_QUEUE_SOURCES
    queue/job_task_queue.cpp
)

# Scheduler's
set(JOBTHREADS_SCHED_HEADERS
    sched/job_isched_policy.h
    sched/job_fifo_scheduler.h
    sched/job_round_robin_scheduler.h
    sched/job_work_stealing_scheduler.h
    sched/job_sporadic_scheduler.h
    sched/job_scheduler_types.h
)
set(JOBTHREADS_SCHED_SOURCES
    sched/job_fifo_scheduler.cpp
    sched/job_round_robin_scheduler.cpp
    sched/job_work_stealing_scheduler.cpp
    sched/job_sporadic_scheduler.cpp
)

# Descriptor's
set(JOBTHREADS_DESCR_HEADERS
    descr/job_idescriptor.h
    descr/job_task_descriptor.h
    descr/job_sporadic_descriptor.h
)
set(JOBTHREADS_DESCR_SOURCES
    descr/job_idescriptor.cpp
)

# ASYNC IO
set(JOBTHREADS_IO_HEADERS
    io/job_io_event.h
    io/job_async_event_loop.h
    io/job_io_async_thread.h
)
set(JOBTHREADS_IO_SOURCES
    io/job_async_event_loop.cpp
    io/job_io_async_thread.cpp

)

# Api The Simple I just want to use threads.
set(JOBTHREADS_CTX_HEADERS
    ctx/job_fifo_ctx.h
    ctx/job_rr_ctx.h
    ctx/job_sporadic_ctx.h
    ctx/job_stealing_ctx.h
)
set(JOBTHREADS_CTX_SOURCES)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Threads SHARED
        ${JOBTHREADS_HEADERS}       ${JOBTHREADS_SOURCES}
        ${JOBTHREADS_UTILS_HEADERS} ${JOBTHREADS_UTILS_SOURCES}
        ${JOBTHREADS_QUEUE_HEADERS} ${JOBTHREADS_QUEUE_SOURCES}
        ${JOBTHREADS_SCHED_HEADERS} ${JOBTHREADS_SCHED_SOURCES}
        ${JOBTHREADS_DESCR_HEADERS} ${JOBTHREADS_DESCR_SOURCES}
        ${JOBTHREADS_IO_HEADERS}    ${JOBTHREADS_IO_SOURCES}
        ${JOBTHREADS_CTX_HEADERS}   ${JOBTHREADS_CTX_SOURCES}
    )
else()
    add_library(JosephsOddBuilder_Threads STATIC
        ${JOBTHREADS_HEADERS}       ${JOBTHREADS_SOURCES}
        ${JOBTHREADS_UTILS_HEADERS} ${JOBTHREADS_UTILS_SOURCES}
        ${JOBTHREADS_QUEUE_HEADERS} ${JOBTHREADS_QUEUE_SOURCES}
        ${JOBTHREADS_SCHED_HEADERS} ${JOBTHREADS_SCHED_SOURCES}
        ${JOBTHREADS_DESCR_HEADERS} ${JOBTHREADS_DESCR_SOURCES}
        ${JOBTHREADS_IO_HEADERS}    ${JOBTHREADS_IO_SOURCES}
        ${JOBTHREADS_CTX_HEADERS}   ${JOBTHREADS_CTX_SOURCES}
    )
    add_compile_definitions(JOBTHREADS_STATIC_DEFINE)
endif()

set(JOBTHREADS_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobthreads_export.h)
generate_export_header(JosephsOddBuilder_Threads
    BASE_NAME JOBTHREADS
    EXPORT_FILE_NAME "${JOBTHREADS_EXPORT}"
)

target_compile_options(JosephsOddBuilder_Threads PRIVATE
    -freflection
    -fcontracts)

target_link_options(JosephsOddBuilder_Threads PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_Threads PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobthreads"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(JosephsOddBuilder_Threads PUBLIC
    atomic
    Threads::Threads
    JosephsOddBuilder_Core
)

target_include_directories(JosephsOddBuilder_Threads PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utils>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/queue>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sched>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/descr>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/io>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ctx>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    ## install paths
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobthreads>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/utils>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/queue>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/sched>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/descr>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/io>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/ctx>
)

install(TARGETS JosephsOddBuilder_Threads
    EXPORT JosephsOddBuilderThreadsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBTHREADS_EXPORT}          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads )
install(FILES ${JOBTHREADS_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads )
install(FILES ${JOBTHREADS_UTILS_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/utils )
install(FILES ${JOBTHREADS_QUEUE_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/queue )
install(FILES ${JOBTHREADS_SCHED_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/sched )
install(FILES ${JOBTHREADS_DESCR_HEADERS}   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/descr )
install(FILES ${JOBTHREADS_IO_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/io )
install(FILES ${JOBTHREADS_CTX_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/ctx )


set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobthreads.pc")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobthreads.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)
install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")


install(EXPORT JosephsOddBuilderThreadsTargets
    FILE JosephsOddBuilderThreadsTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderThreads
)
