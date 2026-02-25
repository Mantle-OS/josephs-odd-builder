cmake_minimum_required(VERSION 3.16)

# find_package(Boost REQUIRED)
cmake_policy(SET CMP0167 NEW) # crazy boost stuff
find_package(msgpack-cxx REQUIRED)
get_target_property(MSGPACK_CXX_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BOOST_LIBS Boost::boost INTERFACE_LINK_LIBRARIES)

add_executable(test_job_serializer_msgpack
    main.cpp
    ../job_serializer/test_emitter.h
    test_job_emitter_msgpack.cpp
    test_job_msgpack.cpp
)

target_link_libraries(test_job_serializer_msgpack PRIVATE
    Threads::Threads
    ${CatchTwo_LIBRARIES}
    ${YAMLCpp_LIBRARIES}
    msgpack-cxx
    Boost::boost
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Serializer
    JosephsOddBuilder_Serializer_MsgPack
)

target_include_directories(test_job_serializer_msgpack PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
    ${LibMsgPackC_INCLUDE_DIRS}
    ${YAMLCpp_INCLUDE_DIRS}
    ${NlohmannJson_INCLUDE_DIRS}
)


add_test(NAME job_serializer_msgpack COMMAND test_job_serializer_msgpack)
