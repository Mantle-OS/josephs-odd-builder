project(JosephOddBuild_Core
    VERSION 0.1
    DESCRIPTION "Core Lib for Joseph's Odd Builder"
    LANGUAGES CXX
)


set(JOB_BINARY_HEADERS
    binary/job_binary.h
)
set(JOB_SIMD_SOURCES
    simd/simd_provider.cpp
)
set(JOB_SIMD_HEADERS
    simd/rounding_mode.h
    simd/transpose.h
    simd/simd_provider.h
    simd/simd_math.h
)

# these LOAD when the providor is used
if(JOB_AVX_512_VNNI_FLAG OR JOB_AVX_512_FLAG)
    list(APPEND  JOB_SIMD_HEADERS simd/simd_avx512.h)
elseif(JOB_AVX_FLAG OR JOB_AVX_TWO_FLAG OR JOB_AVX_VNNI_FLAG)
    list(APPEND  JOB_SIMD_HEADERS simd/simd_avx.h)
elseif(JOB_AVX_NEON_FLAG)
    list(APPEND  JOB_SIMD_HEADERS simd/simd_arm.h)
else()
    message(FATAL_ERROR "AVX mismatch you have to set it up. see cmake/build_options")
endif()

set(JOB_YAML_HEADERS
    # Core parser vocabulary
    yaml/job_yaml_contracts.h
    yaml/job_yaml_concepts.h
    yaml/job_yaml_grammar.h
    yaml/job_yaml_cursor.h

    yaml/job_yaml_lex_type.h
    yaml/job_yaml_lex.h

    # Structural parsing
    yaml/job_yaml_indent_stack.h
    yaml/job_yaml_scalar_kernel.h
    yaml/job_yaml_escape_kernel.h
    yaml/job_yaml_source_range.h

    # Reflection/direct object path
    yaml/job_yaml_key_dispatch.h
    yaml/job_yaml_object_reader.h

    # Output
    yaml/job_yaml_emitter.h

    # Generic parse destinations
    yaml/job_yaml_sink.h
    yaml/job_yaml_node_sink.h
    yaml/job_yaml_object_sink.h
    yaml/job_yaml_validate_sink.h
    yaml/job_yaml_event_sink.h

    # Dynamic representation
    yaml/job_yaml_node_properties.h
    yaml/job_yaml_anchor_table.h
    yaml/job_yaml_alias_stack.h
    yaml/job_yaml_node.h

    # scalar
    yaml/job_yaml_scalar_style.h
    yaml/job_yaml_scalar_decoder.h
    yaml/job_yaml_decoded_scalar.h
    yaml/job_yaml_block_scalar_decoder.h

    # parser madness
    yaml/job_yaml_parse_context.h
    yaml/job_yaml_parser_state.h
    yaml/job_yaml_parser_destination.h
    yaml/job_yaml_object_parser_destination.h
    yaml/job_yaml_parser_utils.h
    yaml/job_yaml_parser.h


    yaml/job_yaml_diagnostic.h

    # API
    yaml/job_yaml.h
)

set(JOB_YAML_SOURCES
    yaml/job_yaml.cpp
)

set(JOB_JSON_HEADERS
    # base
    json/job_json_concepts.h
    json/job_json_contracts.h
    json/job_json_source_range.h

    # vocabulary and cursor
    json/job_json_grammar.h
    json/job_json_cursor.h

    ## lexer
    json/job_json_lex_type.h
    json/job_json_lex.h

    # kernels
    json/job_json_number_kernel.h
    json/job_json_escape_kernel.h
    json/job_json_string_kernel.h

    ## information
    json/job_json_diagnostic.h
    # job_json_parser_state.h

    # parser madness
    json/job_json_key_dispatch.h
    json/job_json_parser_destination.h
    json/job_json_parser.h

    json/job_json_emitter.h

    # API
    json/job_json.h
)

set(JOB_JSON_SOURCES
    # Empty implementation unit used to keep job_json as a compiled library.
    # See job_json.cpp for why this is currently necessary.
    json/job_json.cpp
)

set(JOB_CORE_SOURCES
    job_contract.cpp
    job_logger.cpp
    job_timer.cpp
    job_siphash.cpp

    job_base_obj.cpp
    job_connection.cpp
    job_light_object.cpp
    job_object.cpp
)

set(JOB_CORE_HEADERS
    crc32.h
    endian_utils.h
    real_type.h

    job_contract.h
    job_logger.h

    job_timer.h
    split_mix64.h
    aligned_allocator.h

    job_siphash.h
    job_container_concept.h
    job_list.h
    job_hash_container.h
    job_map.h
    job_obj_hash_concept.h
    job_obj_hash.h

    job_obj_concept.h
    job_base_obj.h
    job_connection.h
    job_signal.h
    job_obj_annotation.h

    job_light_object.h
    job_object.h
)



set(JOB_LATTICE_SOURCES)
set(JOB_LATTICE_HEADERS)

set(JOB_TEXT_SOURCES)
set(JOB_TEXT_HEADERS)


set(JOBCORE_COMPILE_THIS
    ${JOB_SIMD_HEADERS}     ${JOB_SIMD_SOURCES}
    ${JOB_CORE_HEADERS}     ${JOB_CORE_SOURCES}
    ${JOB_YAML_HEADERS}     ${JOB_YAML_SOURCES}
    ${JOB_JSON_HEADERS}     ${JOB_JSON_SOURCES}
    ${JOB_BINARY_HEADERS}
)


if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Core SHARED ${JOBCORE_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Core STATIC ${JOBCORE_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Core PUBLIC JOBCORE_STATIC_DEFINE)
endif()

set(JOBCORE_EXPORT ${CMAKE_CURRENT_BINARY_DIR}/jobcore_export.h)
generate_export_header(JosephsOddBuilder_Core
    BASE_NAME JOBCORE
    EXPORT_FILE_NAME "${JOBCORE_EXPORT}"
)

target_compile_options(JosephsOddBuilder_Core PRIVATE
    -freflection
    -fcontracts)

target_link_options(JosephsOddBuilder_Core PRIVATE
    -fcontracts
)

set_target_properties(JosephsOddBuilder_Core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobcore"
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/simd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/simd>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/yaml>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/yaml>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/json>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/json>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/binary>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/binary>

    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(JosephsOddBuilder_Core PUBLIC
    ${YAMLCpp_LIBRARIES}
)

install(TARGETS JosephsOddBuilder_Core
    EXPORT JosephsOddBuilderCoreTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)


install(FILES ${JOB_CORE_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore)
install(FILES ${JOBCORE_EXPORT}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore)
install(FILES ${JOB_SIMD_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/simd)
install(FILES ${JOB_BINARY_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/binary)
install(FILES ${JOB_YAML_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/yaml)
install(FILES ${JOB_JSON_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/json)


set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobcore.pc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/jobcore.pc.in"
    "${PKGCONFIG_FILE}"
    @ONLY
)

install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}")


install(EXPORT JosephsOddBuilderCoreTargets
    FILE JosephsOddBuilderCoreTargets.cmake
    NAMESPACE JOB::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderCore
)
