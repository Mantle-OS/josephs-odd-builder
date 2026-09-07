project(JosephsOddBuilder_Schema
    VERSION 0.1
    DESCRIPTION "Schema layer for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOB_SCHEMA_HEADERS
    job_schema_types.h
    job_schema.h
    job_schema_concepts.h
    job_schema_contracts.h
    job_schema_reader.h
    job_schema_writer.h
    job_schema_emitter.h
)

set(JOB_SCHEMA_SOURCES
    job_schema.cpp
    job_schema_reader.cpp
    job_schema_writer.cpp
    job_schema_emitter.cpp
)


if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Schema SHARED ${JOB_SCHEMA_HEADERS} ${JOB_SCHEMA_SOURCES})
else()
    add_library(JosephsOddBuilder_Schema STATIC ${JOB_SCHEMA_HEADERS} ${JOB_SCHEMA_SOURCES})
    target_compile_definitions(JosephsOddBuilder_Schema PUBLIC JOBSCHEMA_STATIC_DEFINE)
endif()

set(JOBSCHEMA_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobschema_export.h)
generate_export_header(JosephsOddBuilder_Schema
    BASE_NAME JOBSCHEMA
    EXPORT_FILE_NAME "${JOBSCHEMA_EXPORT}"
)

target_compile_options(JosephsOddBuilder_Schema PRIVATE
    -freflection
    -fcontracts
)

target_link_options(JosephsOddBuilder_Schema PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_Schema PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "job_schema"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Schema PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_schema>
    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_include_directories(JosephsOddBuilder_Schema PRIVATE
    ${YAMLCpp_INCLUDE_DIRS}
    ${NlohmannJson_INCLUDE_DIRS}
)

target_link_libraries(JosephsOddBuilder_Schema PRIVATE
    ${YAMLCpp_LIBRARIES}
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
)

install(TARGETS JosephsOddBuilder_Schema
    EXPORT JosephsOddBuilderSchemaTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBSCHEMA_EXPORT}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_schema)
install(FILES ${JOB_SCHEMA_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_schema)

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_schema.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobschema.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )
    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")
endif()

install(EXPORT JosephsOddBuilderSchemaTargets
    FILE JosephsOddBuilderSchemaTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSchema
)
