project(JosephOddBuild_Net
    VERSION 0.1
    DESCRIPTION "Networking I/O abstraction layer for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOBNET_HEADERS
    isocket_io.h
    job_iana.h
    job_http_header.h
    job_url.h
)

set(JOBNET_SOURCES
    job_url.cpp
    job_http_header.cpp
    isocket_io.cpp
)



set(JOBNET_AUTH_HEADERS
    auth/job_http_auth_type.h
    auth/job_ihttp_auth.h

    auth/job_bearer_auth.h
    auth/job_token_auth.h
    auth/job_api_key_auth.h
    auth/job_basic_auth.h
    auth/job_custom_auth.h

    auth/job_aws4_auth.h

    auth/job_http_auth_factory.h
)

set(JOBNET_AUTH_SOURCES
    auth/job_ihttp_auth.cpp

    auth/job_bearer_auth.cpp
    auth/job_token_auth.cpp
    auth/job_api_key_auth.cpp
    auth/job_basic_auth.cpp
    auth/job_custom_auth.cpp
    auth/job_aws4_auth.cpp
    auth/job_http_auth_factory.cpp
)

set(JOBNET_HTTP_HEADERS
    http/job_http_method.h
    http/job_http_status_code.h
    http/job_http_status.h
    ## Blocked till auth is done
    http/job_http_request.h
    http/job_http_response.h
    http/job_http_parser.h
    http/job_http_client.h
)
set(JOBNET_HTTP_SOURCES
    http/job_http_status.cpp
    ## Blocked till auth is done
    http/job_http_request.cpp
    http/job_http_response.cpp
    http/job_http_parser.cpp
    http/job_http_client.cpp
)

set(JOBNET_RESOLVE_HEADERS
    resolve/job_ipaddr.h
    resolve/job_resolver.h
    resolve/job_ssl_context.h
    resolve/job_ssl_error.h
    resolve/job_proxy_config.h
)

if(JOB_WINDOWS)
    set(JOBNET_RESOLVE_SOURCES
        resolve/win32/job_ipaddr_win32.cpp
        resolve/win32/job_resolver_win32.cpp
        resolve/win32/job_ssl_error_win32.cpp       # schannel mappings back to our errors
        resolve/win32/job_ssl_context_win32.cpp     # schannel context
    )
elseif(JOB_LINUX)
    set(JOBNET_RESOLVE_SOURCES
        resolve/linux/job_ipaddr_linux.cpp
        resolve/linux/job_resolver_linux.cpp

        resolve/linux/job_ssl_error_linux.cpp       # openssl mappings back to our errors
        resolve/linux/job_ssl_context_linux.cpp     # openssl context
    )
else()
    message(FATAL_ERROR "Unsuppored OS for networking. only linux and windows is currently supported")
endif()

set(JOBNET_SOCKETS_HEADERS
    sockets/job_socket_error.h
    sockets/tcp_socket.h
    sockets/udp_socket.h
    sockets/unix_socket.h
    sockets/ssl_socket.h
)

set(JOBNET_SOCKETS_SOURCES)

if(JOB_WINDOWS)
    list(APPEND JOBNET_SOCKETS_SOURCES
        sockets/win32/job_socket_error_win32.cpp
        sockets/win32/tcp_socket_win32.cpp
        sockets/win32/udp_socket_win32.cpp
        sockets/win32/unix_socket_win32.cpp
        sockets/win32/ssl_socket_win32.cpp
    )

elseif(JOB_LINUX)
    list(APPEND JOBNET_SOCKETS_SOURCES
        sockets/linux/job_socket_error_linux.cpp
        sockets/linux/tcp_socket_linux.cpp
        sockets/linux/udp_socket_linux.cpp
        sockets/linux/unix_socket_linux.cpp
        sockets/linux/ssl_socket_linux.cpp
    )
endif()

set(JOBNET_SERVERS_HEADERS
    servers/tcp_server.h
    servers/udp_server.h
    servers/unix_socket_server.h
    servers/ssl_server.h
)

set(JOBNET_SERVERS_SOURCES
    servers/tcp_server.cpp
    servers/udp_server.cpp
    servers/unix_socket_server.cpp
    servers/ssl_server.cpp
)

set(JOBNET_CLIENTS_HEADERS
    clients/tcp_client.h
    clients/udp_client.h
    clients/unix_socket_client.h
    clients/ssl_client.h
)

set(JOBNET_CLIENTS_SOURCES
    clients/tcp_client.cpp
    clients/udp_client.cpp
    clients/unix_socket_client.cpp
    clients/ssl_client.cpp
)


set(JOB_NET_COMPILE_THIS
    ${JOBNET_HEADERS}           ${JOBNET_SOURCES}
    ${JOBNET_RESOLVE_HEADERS}   ${JOBNET_RESOLVE_SOURCES}
    ${JOBNET_SOCKETS_HEADERS}   ${JOBNET_SOCKETS_SOURCES}
    ${JOBNET_SERVERS_HEADERS}   ${JOBNET_SERVERS_SOURCES}
    ${JOBNET_CLIENTS_HEADERS}   ${JOBNET_CLIENTS_SOURCES}
    ${JOBNET_HTTP_HEADERS}      ${JOBNET_HTTP_SOURCES}
    ${JOBNET_AUTH_HEADERS}      ${JOBNET_AUTH_SOURCES}
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Net SHARED ${JOB_NET_COMPILE_THIS}
    )
else()
    add_library(JosephsOddBuilder_Net STATIC ${JOB_NET_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Net PUBLIC JOBNET_STATIC_DEFINE)
endif()


set(JOBNET_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobnet_export.h)
generate_export_header(JosephsOddBuilder_Net
    BASE_NAME JOBNET
    EXPORT_FILE_NAME "${JOBNET_EXPORT}"
)
target_compile_options(JosephsOddBuilder_Net PRIVATE
    -freflection
    -fcontracts)

target_link_options(JosephsOddBuilder_Net PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_Net PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobnet"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Net PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobnet>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/resolve>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobnet/resolve>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/sockets>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobnet/sockets>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/clients>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobnet/clients>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/servers>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobnet/servers>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/http>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobnet/http>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/auth>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobnet/auth>

    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

## FIXME one day soon I need to go through these and fix the whole public vs private deal for the export
if(JOB_WINDOWS)
    target_link_libraries(JosephsOddBuilder_Net PRIVATE
        ws2_32
        advapi32
        secur32
        dnsapi
        iphlpapi
        winhttp
    )
    target_compile_definitions(JosephsOddBuilder_Net PRIVATE NOMINMAX)

elseif(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Net PRIVATE
        ${LibOpenSSL_LIBRARIES}
        ${LibCrypto_LIBRARIES}
        atomic
    )
endif()

target_link_libraries(JosephsOddBuilder_Net PRIVATE
    Threads::Threads
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Crypto
)

install(TARGETS JosephsOddBuilder_Net
    EXPORT JosephsOddBuilderNetTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet
)

install(FILES ${JOBNET_EXPORT}              DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet )
install(FILES ${JOBNET_HEADERS}             DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet )
install(FILES ${JOBNET_RESOLVE_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/resolve )
install(FILES ${JOBNET_SOCKETS_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/sockets )
install(FILES ${JOBNET_CLIENTS_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/clients )
install(FILES ${JOBNET_SERVERS_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/servers )
install(FILES ${JOBNET_HTTP_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/http )
install(FILES ${JOBNET_HTTP_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobnet/auth )

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobnet.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobnet.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT JosephsOddBuilderNetTargets
    FILE JosephsOddBuilderNetTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderNet
)

