project(JosephsOddBuilder_Serializer_Flatbuffers
    VERSION 0.1
    DESCRIPTION "Flatbuffers plugin (Serializer) for Joseph's Odd Builder"
    LANGUAGES CXX
)

# sudo apt install libflatbuffers-dev flatbuffers-compiler
pkg_check_modules(Flatbuffers REQUIRED flatbuffers)

set(JOBSER_FLATBUFFERS_HEADERS
    job_serializer_flatbuffers.h
)

set(JOBSER_FLATBUFFERS_SOURCES
    job_serializer_flatbuffers.cpp
)

add_library(JosephsOddBuilder_Serializer_Flatbuffers SHARED
    ${JOBSER_FLATBUFFERS_SOURCES}
    ${JOBSER_FLATBUFFERS_HEADERS}
)

target_link_libraries(JosephsOddBuilder_Serializer_Flatbuffers PUBLIC
    ${Flatbuffer_LIBRARIES}
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Serializer
)

set_target_properties(JosephsOddBuilder_Serializer_Flatbuffers PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobserializerflatbuffers"
)

target_include_directories(JosephsOddBuilder_Serializer_Flatbuffers PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
)

target_include_directories(JosephsOddBuilder_Serializer_Flatbuffers PRIVATE
    ${YAMLCpp_INCLUDE_DIRS}
    ${NlohmannJson_INCLUDE_DIRS}
    ${Flatbuffers_INCLUDE_DIRS}
)

target_link_libraries(JosephsOddBuilder_Serializer_Flatbuffers PRIVATE
    ${YAMLCpp_LIBRARIES}
    ${Flatbuffers_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Serializer
)

install(TARGETS JosephsOddBuilder_Serializer_Flatbuffers
    EXPORT JosephsOddBuilderSerializerFlatbuffersTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBSER_FLATBUFFERS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobserializerflatbuffers
)

set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobserializerflatbuffers.pc")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobserializerflatbuffers.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
install(EXPORT JosephsOddBuilderSerializerFlatbuffersTargets
    FILE JosephsOddBuilderSerializerFlatbuffersTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSerializerFlatbuffers
)

# CHECKPOINT: v1
