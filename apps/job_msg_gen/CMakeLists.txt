cmake_minimum_required(VERSION 3.16)

project(job_msg_gen
    VERSION 0.1
    DESCRIPTION "Message pack compiler."
    LANGUAGES CXX
)

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED)
find_package(msgpack-cxx REQUIRED)

add_executable(job_msg_gen
    main.cpp
)

set_target_properties(job_msg_gen PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(job_msg_gen
    PRIVATE
        -freflection
        -fcontracts
)

target_link_libraries(job_msg_gen
    PRIVATE
        Threads::Threads
        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost
        JosephsOddBuilder_Core
        JosephsOddBuilder_Threads
        JosephsOddBuilder_IO
        JosephsOddBuilder_Serializer
        JosephsOddBuilder_Serializer_MsgPack
)

install(TARGETS job_msg_gen
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)