cmake_minimum_required(VERSION 3.16)
project(job-reflection-test
    VERSION 0.1
    DESCRIPTION "testing c++ 26 and reflection"
    LANGUAGES CXX
)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++26>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-freflection>)

add_executable(job-reflection-test
    ping.h         # Ping : public Object
    pong.h         # Pong : public Object

    ser_obj_concept.h
    ser_obj.h
    ser_nested_obj.h

    tmp_file.h
    test_loop.h

    packed.h
    ipc_mmap.h
    ipc_shm.h
    ipc_tcp.h
    ipc_unix.h #trash for now
    ipc_ssl.h
    ipc_udp.h
    main.cpp
)
# rec.h

# bring in things that need c++23 as a test and also just some randoms
target_compile_options(job-reflection-test PRIVATE
    -freflection
    -fcontracts)

target_link_options(job-reflection-test PRIVATE
    -fcontracts
)

set_target_properties(job-reflection-test PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(job-reflection-test
    PRIVATE
        Threads::Threads
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads

        ${YAMLCpp_LIBRARIES}
        $<BUILD_INTERFACE:ggml>
        JosephsOddBuilder_Ggml

        JosephsOddBuilder_Token
        JosephsOddBuilder_IO
        JosephsOddBuilder_Net
        JosephsOddBuilder_Crypto
)

install(TARGETS job-reflection-test
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
