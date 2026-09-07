cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0167 NEW)

find_package(Boost REQUIRED)
find_package(msgpack-cxx REQUIRED)

add_executable(test_job_serializer_msgpack
    main.cpp
    ../job_serializer/test_emitter.h
    test_job_emitter_msgpack.cpp
    test_job_msgpack.cpp
)

set_target_properties(test_job_serializer_msgpack PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(test_job_serializer_msgpack
    PRIVATE
        -freflection
        -fcontracts
)

target_link_libraries(test_job_serializer_msgpack
    PRIVATE
        Threads::Threads
        ${CatchTwo_LIBRARIES}
        ${YAMLCpp_LIBRARIES}
        msgpack-cxx
        Boost::boost
        JosephsOddBuilder_Core
        JosephsOddBuilder_IO
        JosephsOddBuilder_Serializer
        JosephsOddBuilder_Serializer_MsgPack
)

target_include_directories(test_job_serializer_msgpack
    PRIVATE
        ${CatchTwo_INCLUDE_DIRS}
        ${YAMLCpp_INCLUDE_DIRS}
        ${NlohmannJson_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../job_serializer
)

add_test(
    NAME job_serializer_msgpack
    COMMAND test_job_serializer_msgpack
)