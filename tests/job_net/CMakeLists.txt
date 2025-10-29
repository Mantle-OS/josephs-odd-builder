cmake_minimum_required(VERSION 3.16)

add_executable(test_job_net
    main.cpp
    test_url.cpp
    test_ipaddr.cpp
    test_http_header.cpp
    test_tcp_socket.cpp
    test_udp_socket.cpp
    test_unix_socket.cpp
    # tcp_stack.cpp
)

target_link_libraries(test_job_net PRIVATE
    Threads::Threads
    ${CatchTwo_LIBRARIES}
    ${LibOpenSSL_LIBRARIES}
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Crypto
    JosephsOddBuilder_Net
)

target_include_directories(test_job_net PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)
