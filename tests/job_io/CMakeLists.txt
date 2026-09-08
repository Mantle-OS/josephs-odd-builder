cmake_minimum_required(VERSION 3.16)


option(JOB_TEST_IO_PTY_DEBUG_SER OFF)
if(JOB_TEST_IO_PTY_DEBUG_SER)
    add_compile_definitions(JOB_TEST_IO_PTY_DEBUG_SER)
endif()

add_executable(test_job_io
    ../transient_test_file.h
    main.cpp

    ## START_PTY
    test_pty_env.cpp
    test_pty_env_posix.cpp
    test_pty_env_xdg.cpp
    test_pty_dot.cpp
    test_pty_environ.cpp
    test_pty.cpp
    test_process.cpp
    # ## END_PTY

    test_permissions.cpp
    test_job_dir.cpp
    test_job_tmp_dir.cpp
    test_job_file.cpp
    test_job_tmp_file.cpp

    test_job_syslink.cpp

    # Xdg
    test_job_xdg_paths.cpp
    test_job_xdg_entry.cpp
    test_xdg_desktop_entry.cpp
    test_job_xdg.cpp
    #end xdg

    test_fileio.cpp
    test_mockio.cpp
    test_job_shared_memory.cpp


    ## START MEMORY
    test_job_mem_lock.cpp
    test_job_mem_descriptors.cpp
    test_job_mmap.cpp
    test_job_range_pool.cpp
    test_job_page_pool.cpp
    test_job_size_pool.cpp
    test_job_arena_pool.cpp

    test_io_factory.cpp
)

# bring in things that need c++23 as a test and also just some randoms
target_compile_options(test_job_io PRIVATE
    -freflection
    -fcontracts)

target_link_options(test_job_io PRIVATE
    -fcontracts
)

set_target_properties(test_job_io PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(test_job_io PRIVATE
    Threads::Threads
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Crypto
    JosephsOddBuilder_IO
)

target_include_directories(test_job_io PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_io COMMAND test_job_io)
