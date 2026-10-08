project(JosephOddBuild_Core
    VERSION 0.1
    DESCRIPTION "Core Lib for Joseph's Odd Builder"
    LANGUAGES CXX
)

set(JOB_CORE_HEADERS
    crc32.h
    endian_utils.h
    real_type.h

    job_contract.h
    job_logger.h

    job_timer.h
    aligned_allocator.h

    split_mix64.h
    job_siphash.h

    job_string_utils.h


)

set(JOB_CORE_SOURCES
    job_contract.cpp
    job_logger.cpp
    job_timer.cpp
    job_siphash.cpp
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
    yaml/job_yaml_contracts.h
    yaml/job_yaml_concepts.h
    yaml/job_yaml_grammar.h
    yaml/job_yaml_cursor.h

    yaml/job_yaml_lex_type.h
    yaml/job_yaml_lex.h

    yaml/job_yaml_indent_stack.h
    yaml/job_yaml_scalar_kernel.h
    yaml/job_yaml_escape_kernel.h
    yaml/job_yaml_source_range.h

    yaml/job_yaml_key_dispatch.h
    yaml/job_yaml_object_reader.h

    yaml/job_yaml_emitter.h

    yaml/job_yaml_sink.h
    yaml/job_yaml_node_sink.h
    yaml/job_yaml_object_sink.h
    yaml/job_yaml_validate_sink.h
    yaml/job_yaml_event_sink.h

    yaml/job_yaml_node_properties.h
    yaml/job_yaml_anchor_table.h
    yaml/job_yaml_alias_stack.h
    yaml/job_yaml_node.h

    yaml/job_yaml_scalar_style.h
    yaml/job_yaml_scalar_decoder.h
    yaml/job_yaml_decoded_scalar.h
    yaml/job_yaml_block_scalar_decoder.h

    yaml/job_yaml_parse_context.h
    yaml/job_yaml_parser_state.h
    yaml/job_yaml_parser_destination.h
    yaml/job_yaml_object_parser_destination.h
    yaml/job_yaml_parser_utils.h
    yaml/job_yaml_parser.h

    yaml/job_yaml_diagnostic.h

    yaml/job_yaml.h
)

set(JOB_YAML_SOURCES
    yaml/job_yaml.cpp
)

set(JOB_JSON_HEADERS
    json/job_json_concepts.h
    json/job_json_contracts.h
    json/job_json_source_range.h

    json/job_json_grammar.h
    json/job_json_cursor.h

    json/job_json_lex_type.h
    json/job_json_lex.h

    json/job_json_number_kernel.h
    json/job_json_escape_kernel.h
    json/job_json_string_kernel.h

    json/job_json_diagnostic.h
    # job_json_parser_state.h

    json/job_json_key_dispatch.h
    json/job_json_parser_destination.h
    json/job_json_parser.h

    json/job_json_emitter.h

    json/job_json.h
)

set(JOB_JSON_SOURCES
    json/job_json.cpp
)



set(JOB_OBJ_HEADERS
    obj/job_obj_concept.h
    obj/job_base_obj.h
    obj/job_connection.h
    obj/job_signal.h
    obj/job_obj_annotation.h

    obj/job_light_object.h
    obj/job_object.h

    obj/job_obj_grammar.h
    obj/job_obj_members.h
    obj/job_obj_ptr.h
    obj/job_obj_class.h

    obj/job_obj.h
)

set(JOB_OBJ_SOURCES
    obj/job_base_obj.cpp
    obj/job_connection.cpp
    obj/job_light_object.cpp
    obj/job_object.cpp
)





set(JOB_LATTICE_SOURCES)
set(JOB_LATTICE_HEADERS
    lattice/job_lattice_coordinate.h
    lattice/job_lattice_point.h

    lattice/job_lattice_concepts.h
    lattice/job_lattice_transform_result.h
    lattice/job_lattice_view.h

    lattice/job_lattice_codec.h
    lattice/job_lattice_kernel.h

    lattice/job_lattice_contracts.h

    lattice/job_lattice.h
)

set(JOB_TEXT_SOURCES
    text/job_utf_streambuf.cpp
)
set(JOB_TEXT_HEADERS
    text/job_unicode_scalar.h

    # Utf8
    text/job_utf8_status.h
    text/job_utf8_scalar_kernel.h
    text/job_utf8_kernel.h
    text/job_utf8_simd_tile.h
    text/job_utf8_simd_kernel.h
    text/job_ascii_simd_tile.h
    text/job_ascii_simd_kernel.h
    text/job_utf8.h

    # Utf16
    text/job_utf16_status.h
    text/job_utf16_scalar_kernel.h
    text/job_utf16_kernel.h
    text/job_utf16_simd_tile.h
    text/job_utf16_simd_kernel.h
    text/job_utf16.h

    # Utf32
    text/job_utf32_status.h
    text/job_utf32_scalar_kernel.h
    text/job_utf32_kernel.h
    text/job_utf32_simd_tile.h
    text/job_utf32_simd_kernel.h
    text/job_utf32.h

    ## Higher level api usage
    text/job_text_encoding.h
    text/job_text_transcode_status.h
    text/job_text_transcoder.h
    text/job_text_dispatch.h

    text/job_utf_stream_concept.h # Hello
    text/job_utf_streambuf.h



    text/job_text.h
)

set(JOB_GFX_SOURCES

)

set(JOB_GFX_HEADERS
    gfx/job_rect.h
    gfx/job_rgb.h
    gfx/job_color_channel.h
    gfx/job_color_channel_info.h
    gfx/job_render_format_type.h
    gfx/job_render_format.h
    gfx/job_color_type.h
    gfx/job_color_concept.h
    gfx/job_ansi_color_utils.h # hello
)

set(JOB_CONTAINER_HEADERS
    container/job_container_concept.h
    container/job_list.h
    container/job_hash_container.h
    container/job_map.h
    container/job_obj_hash_concept.h
    container/job_obj_hash.h
)

set(JOB_CONTAINER_SOURCES

)



set(JOBCORE_COMPILE_THIS
    ${JOB_SIMD_HEADERS}         ${JOB_SIMD_SOURCES}
    ${JOB_CORE_HEADERS}         ${JOB_CORE_SOURCES}
    ${JOB_CONTAINER_HEADERS}    ${JOB_CONTAINER_SOURCES}
    ${JOB_YAML_HEADERS}         ${JOB_YAML_SOURCES}
    ${JOB_JSON_HEADERS}         ${JOB_JSON_SOURCES}
    ${JOB_BINARY_HEADERS}
    ${JOB_LATTICE_HEADERS}
    ${JOB_TEXT_HEADERS}         ${JOB_TEXT_SOURCES}
    ${JOB_OBJ_HEADERS}          ${JOB_OBJ_SOURCES}
    ${JOB_GFX_HEADERS}          ${JOB_GFX_SOURCES}
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/simd>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/simd>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/container>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/container>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/yaml>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/yaml>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/json>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/json>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/binary>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/binary>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lattice>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/lattice>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/text>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/text>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/obj>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/obj>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gfx>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobcore/gfx>

    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
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
install(FILES ${JOB_LATTICE_HEADERS}    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/lattice)
install(FILES ${JOB_TEXT_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/text)
install(FILES ${JOB_OBJ_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/obj)
install(FILES ${JOB_GFX_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/gfx)
install(FILES ${JOB_CONTAINER_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobcore/container)

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
