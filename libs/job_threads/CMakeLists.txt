cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilderThreads
    VERSION 0.1
    LANGUAGES CXX
)

#########################
# CORE
#########################

set(JOBTHREADS_HEADERS
    job_thread.h
    job_thread_pool.h
    job_thread_watcher.h
)

set(JOBTHREADS_SOURCES
    job_thread.cpp
    job_thread_pool.cpp
    job_thread_watcher.cpp
)

#########################
# UTILS
#########################

set(JOBTHREADS_UTILS_HEADERS
    utils/job_thread_options.h
    utils/job_thread_args.h
    utils/job_thread_metrics.h
    utils/job_parallel_for.h
    utils/job_parallel_reduce.h
    utils/job_thread_node.h
    utils/job_thread_graph.h
    utils/job_thread_actor.h
)

set(JOBTHREADS_UTILS_SOURCES
    utils/job_thread_graph.cpp
)

#########################
# QUEUE's
#########################

set(JOBTHREADS_QUEUE_HEADERS
    queue/job_task_queue.h
    queue/job_mcmp_queue.h
)
set(JOBTHREADS_QUEUE_SOURCES
    queue/job_task_queue.cpp
)

#########################
# Scheduler's
#########################

set(JOBTHREADS_SCHED_HEADERS
    sched/job_isched_policy.h
    sched/job_fifo_scheduler.h
)
set(JOBTHREADS_SCHED_SOURCES
    sched/job_fifo_scheduler.cpp
)

#########################
# Descriptor's
#########################

set(JOBTHREADS_DESCR_HEADERS
    descr/job_idescriptor.h
    descr/job_task_descriptor.h
)

set(JOBTHREADS_DESCR_SOURCES
    descr/job_idescriptor.cpp
)

#########################
# ASYNC IO
#########################

set(JOBTHREADS_IO_HEADERS
    io/job_async_event_loop.h
    io/job_io_async_thread.h
)
set(JOBTHREADS_IO_SOURCES
    io/job_async_event_loop.cpp
    io/job_io_async_thread.cpp
)

add_library(JosephsOddBuilder_Threads SHARED
    ${JOBTHREADS_HEADERS}       ${JOBTHREADS_SOURCES}
    ${JOBTHREADS_UTILS_HEADERS} ${JOBTHREADS_UTILS_SOURCES}
    ${JOBTHREADS_QUEUE_HEADERS} ${JOBTHREADS_QUEUE_SOURCES}
    ${JOBTHREADS_SCHED_HEADERS} ${JOBTHREADS_SCHED_SOURCES}
    ${JOBTHREADS_DESCR_HEADERS} ${JOBTHREADS_DESCR_SOURCES}
    ${JOBTHREADS_IO_HEADERS}    ${JOBTHREADS_IO_SOURCES}
)

set_target_properties(JosephsOddBuilder_Threads PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobthreads"
)

target_link_libraries(JosephsOddBuilder_Threads PUBLIC
    Threads::Threads
    JosephsOddBuilder_Core
)
target_include_directories(JosephsOddBuilder_Threads PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>/utils
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>/utils
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>/queue
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>/queue
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>/sched
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>/sched
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>/descr
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>/descr
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>/io
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>/io
)

install(TARGETS JosephsOddBuilder_Threads
    EXPORT JosephsOddBuilderThreadsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBTHREADS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads
)

install(FILES ${JOBTHREADS_UTILS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/utils
)

install(FILES ${JOBTHREADS_QUEUE_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/queue
)

install(FILES ${JOBTHREADS_SCHED_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/sched
)

install(FILES ${JOBTHREADS_DESCR_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/descr
)

install(FILES ${JOBTHREADS_IO_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobthreads/io
)

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

# CHECKPOINT: v1.3
