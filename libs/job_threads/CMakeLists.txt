cmake_minimum_required(VERSION 3.16)
project(JosephsOddBuilderThreads
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW) # for reproducible timestamps
endif()
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

include(GNUInstallDirs)
find_package(Threads REQUIRED) # for real time threads

set(JOBTHREADS_HEADERS
    async_event_loop.h
    job_thread.h
    task_queue.h
    thread_options.h
    thread_pool.h
    thread_watcher.h
)

set(JOBTHREADS_SOURCES
    async_event_loop.cpp
    job_thread.cpp
    task_queue.cpp
    thread_options.cpp
    thread_pool.cpp
    thread_watcher.cpp
)

add_library(JosephsOddBuilder_Threads SHARED
   ${JOBTHREADS_HEADERS}
   ${JOBTHREADS_SOURCES}
)

set_target_properties(JosephsOddBuilder_Threads PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobthreads"
)

target_link_libraries(JosephsOddBuilder_Threads PUBLIC Threads::Threads)
target_include_directories(JosephsOddBuilder_Threads PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
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

