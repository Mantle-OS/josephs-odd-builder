project(JosephsOddBuilder_Serializer
    VERSION 0.1
    DESCRIPTION "Serializer layer for Joseph's Odd Builder"
    LANGUAGES CXX
)

option(MSGPACK_BINDINGS "Enable MsgPack backend" ON)
option(FLATBUFFERS_BINDINGS "Enable FlatBuffers backend" OFF)

set(JOBSER_DEPS
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
)

set(JOBSER_SOURCES
    job_serializer_utils.cpp
    job_field.cpp
    schema.cpp
    reader.cpp
    writer.cpp
    runtime_object.cpp
    iserializer.cpp
    emitters/emitter.cpp
    emitters/cpp_emitter.cpp
)


set(JOBSER_HEADERS
    spdx_helper.h
    job_serializer_utils.h
    job_field.h
    schema.h
    reader.h
    writer.h
    runtime_object.h
    iserializer.h
    emitters/emitter.h
    emitters/cpp_emitter.h
)

if(MSGPACK_BINDINGS)
    add_subdirectory(job_msgpack)
endif()

if(FLATBUFFERS_BINDINGS)
    add_subdirectory(job_flatbuffers)
endif()

add_library(JosephsOddBuilder_Serializer SHARED
    ${JOBSER_HEADERS}
    ${JOBSER_SOURCES}
)

set_target_properties(JosephsOddBuilder_Serializer PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobserializer"
)

target_include_directories(JosephsOddBuilder_Serializer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_include_directories(JosephsOddBuilder_Serializer PRIVATE
    ${YAMLCpp_INCLUDE_DIRS}
    ${NlohmannJson_INCLUDE_DIRS}
)

target_link_libraries(JosephsOddBuilder_Serializer PRIVATE
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_IO
    JosephsOddBuilder_Threads
)

install(TARGETS JosephsOddBuilder_Serializer
    EXPORT JosephsOddBuilderSerializerTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBSER_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobserializer
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobserializer.pc")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobserializer.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderSerializerTargets
    FILE JosephsOddBuilderSerializerTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSerializer
)
# CHECKPOINT: v1
