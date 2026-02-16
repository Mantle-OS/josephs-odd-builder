project(JosephOddBuild_Net
    VERSION 0.1
    DESCRIPTION "I/O abstraction layer for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBNET_SOURCES
    job_url.cpp
    job_ipaddr.cpp
    job_http_header.cpp
    isocket_io.cpp
    #tcp
    tcp_socket.cpp
    #udp
    udp_socket.cpp
    # unix
    unix_socket.cpp
    # ssl
    ssl/job_ssl_cert.cpp
    ssl/ssl_socket.cpp
)
set(JOBNET_HEADERS
    socket_error.h
    job_iana.h
    job_http_header.h
    job_url.h
    job_ipaddr.h
    isocket_io.h
    #tcp
    tcp_socket.h
    #udp
    udp_socket.h
    # unix
    unix_socket.h
    # ssl
    ssl/ssl_error.h
    ssl/job_ssl_cert.h
    ssl/ssl_socket.h
)

set(JOBNET_SERVERS_HEADERS
    servers/tcp_server.h
    servers/udp_server.h
    servers/unix_socket_server.h
)

set(JOBNET_SERVERS_SOURCES
    servers/tcp_server.cpp
    servers/udp_server.cpp
    servers/unix_socket_server.cpp
)

set(JOBNET_CLIENTS_HEADERS
    clients/tcp_client.h
    clients/udp_client.h
    clients/unix_socket_client.h
)

set(JOBNET_CLIENTS_SOURCES
    clients/tcp_client.cpp
    clients/udp_client.cpp
    clients/unix_socket_client.cpp
)

add_library(JosephsOddBuilder_Net SHARED
    ${JOBNET_HEADERS} ${JOBNET_SOURCES}
    ${JOBNET_SERVERS_HEADERS} ${JOBNET_SERVERS_SOURCES}
    ${JOBNET_CLIENTS_HEADERS} ${JOBNET_CLIENTS_SOURCES}
)

set_target_properties(JosephsOddBuilder_Net PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobnet"
)

target_include_directories(JosephsOddBuilder_Net PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_link_libraries(JosephsOddBuilder_Net PRIVATE
    ${LibOpenSSL_LIBRARIES}
    ${LibCrypto_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Crypto
)

install(TARGETS JosephsOddBuilder_Net
    EXPORT JosephsOddBuilderNetTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBNET_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet
)


install(FILES ${JOBNET_SERVERS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/servers
)

install(FILES ${JOBNET_CLIENTS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/clients
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobnet.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobnet.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderNetTargets
    FILE JosephsOddBuilderNetTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderNet
)
# CHECKPOINT: v1.1
