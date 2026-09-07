cmake_minimum_required(VERSION 3.16)

add_executable(test_job_net
    main.cpp
    # Utils and local helper
    ../transient_test_file.h
    test_loop.h
    test_url.cpp
    test_ipaddr.cpp
    test_http_header.cpp
    # sockets
    test_tcp_socket.cpp
    test_udp_socket.cpp
    test_unix_socket.cpp
    test_ssl_socket.cpp
    # # Clients / server testing
    test_tcp_client_server.cpp
    test_udp_client_server.cpp
    test_unix_client_server.cpp
    test_ssl_client_server.cpp
    # # http
    test_http_method.cpp
    test_http_status.cpp
    test_http_request.cpp
    test_http_response.cpp
    test_http_parser.cpp
    test_http_client.cpp

    # # later / independent
    # test_http_auth.cpp
    # test_aws4_signer.cpp

)
target_compile_options(test_job_net PRIVATE -freflection -fcontracts)
target_link_options(test_job_net PRIVATE -fcontracts)

set_target_properties(test_job_net PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
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

add_test(NAME job_net COMMAND test_job_net)
