cmake_minimum_required(VERSION 3.16)

add_executable(test_job_io
    main.cpp
    test_pty.cpp
    test_fileio.cpp
    test_mockio.cpp
    test_serial_settings.cpp
    test_serial_info.cpp
    test_serial_manager.cpp
    test_io_factory.cpp

    test_url.cpp
    test_ipaddr.cpp
    test_http_header.cpp

)

target_link_libraries(test_job_io PRIVATE
    Threads::Threads
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Crypto
)

target_include_directories(test_job_io PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
