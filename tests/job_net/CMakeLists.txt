cmake_minimum_required(VERSION 3.16)

add_executable(test_job_net
    main.cpp
    # Utils and local helper
    test_loop.h
    test_url.cpp
    test_ipaddr.cpp
    test_http_header.cpp
    # sockets
    test_tcp_socket.cpp ## up 1st
    test_udp_socket.cpp
    test_unix_socket.cpp
    # Clients / server testing
    test_tcp_client_server.cpp # up next
    test_udp_client_server.cpp
    test_unix_client_server.cpp
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
# CHECKPOINT: v1.3
