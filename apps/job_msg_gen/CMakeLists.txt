cmake_minimum_required(VERSION 3.16)
project(job_msg_gen
    VERSION 0.1
    DESCRIPTION "Message pack compiler."
    LANGUAGES CXX
)

cmake_policy(SET CMP0167 NEW) # crazy boost stuff
find_package(msgpack-cxx REQUIRED)

get_target_property(MSGPACK_CXX_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BOOST_LIBS Boost::boost INTERFACE_LINK_LIBRARIES)

add_executable(job_msg_gen
    main.cpp
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
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

