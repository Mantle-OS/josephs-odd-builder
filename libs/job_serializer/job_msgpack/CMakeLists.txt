project(JosephsOddBuilder_Serializer_MsgPack
    VERSION 0.1
    DESCRIPTION "Msgpack plugin (Serializer) for Joseph's Odd Builder"
    LANGUAGES CXX
)

# find_package(Boost REQUIRED)
cmake_policy(SET CMP0167 NEW) # crazy boost stuff
find_package(msgpack-cxx REQUIRED)

get_target_property(MSGPACK_CXX_INCLUDE_DIRS msgpack-cxx INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BOOST_LIBS Boost::boost INTERFACE_LINK_LIBRARIES)

set(PKG_EXTRA_CFLAGS "-I${MSGPACK_CXX_INCLUDE_DIRS} -I${Boost_INCLUDE_DIRS}")
set(PKG_EXTRA_LIBS "${BOOST_LIBS}")

set(JOBSER_MSGPACK_HEADERS
    job_util_msgpack.h
    job_serializer_msgpack.h
    job_emitter_msgpack.h
)

set(JOBSER_MSGPACK_SOURCES
    job_util_msgpack.cpp
    job_serializer_msgpack.cpp
    job_emitter_msgpack.cpp
)

add_library(JosephsOddBuilder_Serializer_MsgPack SHARED
    ${JOBSER_MSGPACK_SOURCES}
    ${JOBSER_MSGPACK_HEADERS}
)

set_target_properties(JosephsOddBuilder_Serializer_MsgPack PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobserializermsgpack"
)

target_link_libraries(JosephsOddBuilder_Serializer_MsgPack
    PUBLIC
        JosephsOddBuilder_Serializer
    PRIVATE
        ${YAMLCpp_LIBRARIES}
        JosephsOddBuilder_Core
        JosephsOddBuilder_IO
        JosephsOddBuilder_Threads
        msgpack-cxx
        Boost::boost
)

target_include_directories(JosephsOddBuilder_Serializer_MsgPack PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobserializermsgpack.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobserializermsgpack.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(TARGETS JosephsOddBuilder_Serializer_MsgPack
    EXPORT JosephsOddBuilderSerializerMsgPackTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBSER_MSGPACK_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobserializermsgpack
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")

install(EXPORT JosephsOddBuilderSerializerMsgPackTargets
    FILE JosephsOddBuilderSerializerMsgPackTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSerializerMsgPack
)
# CHECKPOINT: v1
