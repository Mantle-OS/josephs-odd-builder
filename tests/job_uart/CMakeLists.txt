cmake_minimum_required(VERSION 3.16)

add_executable(test_job_uart
    main.cpp
    test_serial_settings.cpp
    test_serial_info.cpp
    test_serial_manager.cpp
)

target_link_libraries(test_job_uart PRIVATE
    Threads::Threads
    ${CatchTwo_LIBRARIES}
    ${LibUdev_LIBRARIES}
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Uart
)

target_include_directories(test_job_uart PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_uart COMMAND test_job_uart)

