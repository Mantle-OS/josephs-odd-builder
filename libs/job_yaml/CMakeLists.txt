project(JosephsOddBuilder_Yaml
    VERSION 0.1
    DESCRIPTION "Yaml layer for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOB_YAML_HEADERS
    # Core parser vocabulary
    job_yaml_contracts.h
    job_yaml_concepts.h
    job_yaml_grammar.h
    job_yaml_cursor.h

    job_yaml_lex_type.h
    job_yaml_lex.h

    # Structural parsing
    job_yaml_indent_stack.h
    job_yaml_scalar_kernel.h
    job_yaml_escape_kernel.h
    job_yaml_source_range.h

    # Reflection/direct object path
    job_yaml_key_dispatch.h
    job_yaml_object_reader.h

    # Output
    job_yaml_emitter.h

    # Generic parse destinations
    job_yaml_sink.h
    job_yaml_node_sink.h
    job_yaml_object_sink.h
    job_yaml_validate_sink.h
    job_yaml_event_sink.h

    # Dynamic representation
    job_yaml_node_properties.h
    job_yaml_anchor_table.h
    job_yaml_alias_stack.h
    job_yaml_node.h

    # scalar
    job_yaml_scalar_style.h
    job_yaml_scalar_decoder.h
    job_yaml_decoded_scalar.h
    job_yaml_block_scalar_decoder.h

    # parser madness
    job_yaml_parse_context.h
    job_yaml_parser_state.h
    job_yaml_parser_destination.h
    job_yaml_object_parser_destination.h
    job_yaml_parser_utils.h
    job_yaml_parser.h


    job_yaml_diagnostic.h

    # API
    job_yaml.h
)

set(JOB_YAML_SOURCES
    job_yaml.cpp
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Yaml SHARED
        ${JOB_YAML_HEADERS}
        ${JOB_YAML_SOURCES}
    )
else()
    add_library(JosephsOddBuilder_Yaml STATIC
        ${JOB_YAML_HEADERS}
        ${JOB_YAML_SOURCES}
    )
    target_compile_definitions(JosephsOddBuilder_Yaml PUBLIC JOBYAML_STATIC_DEFINE)
endif()

set(JOBYAML_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobyaml_export.h)

generate_export_header(JosephsOddBuilder_Yaml
    BASE_NAME JOBYAML
    EXPORT_FILE_NAME "${JOBYAML_EXPORT}"
)

target_compile_options(JosephsOddBuilder_Yaml PRIVATE
    -freflection
    -fcontracts
)

target_link_options(JosephsOddBuilder_Yaml PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_Yaml PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "job_yaml"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Yaml PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/job_yaml>
    # export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

add_library(JOB::Yaml ALIAS JosephsOddBuilder_Yaml)

install(TARGETS JosephsOddBuilder_Yaml
    EXPORT JosephsOddBuilderYamlTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${JOBYAML_EXPORT}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_yaml
)

install(FILES ${JOB_YAML_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/job_yaml
)

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/job_yaml.pc")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobyaml.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )

    install(
        FILES "${PKGCONFIG_FILE}"
        DESTINATION "${PKGCONFIG_INSTALL_DIR}"
    )
endif()

install(EXPORT JosephsOddBuilderYamlTargets
    FILE JosephsOddBuilderYamlTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderYaml
)