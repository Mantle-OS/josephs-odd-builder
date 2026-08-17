project(JosephsOddBuilder_Token
    VERSION 0.1
    LANGUAGES CXX
)


set(JOBTOKEN_HEADERS
    job_tokenizer_types.h
    job_tokenizer_error.h
    job_tokenizer.h
    byte_level.h
)

set(JOBTOKEN_SOURCES
    job_tokenizer.cpp
    byte_level.cpp
)

set(JOBTOKEN_ALGO_HEADERS
    algo/itoken_algo.h
    algo/bpe_algo.h
    algo/unigram_algo.h
    algo/wordpiece_algo.h
    # algo/motif_algo.h
)

set(JOBTOKEN_ALGO_SOURCES
    algo/bpe_algo.cpp
    algo/unigram_algo.cpp
    algo/wordpiece_algo.cpp
)

set(JOBTOKEN_CORE_HEADERS
    core/byte_lattice.h
    core/byte_lattice_kernel.h
    core/trie.h
    core/unicode_normalizer.h
    core/regex_splitter.h
    core/byte_fallback.h
)

set(JOBTOKEN_CORE_SOURCES
    core/regex_splitter.cpp
    core/unicode_normalizer.cpp
)

set(JOBTOKEN_FORMATS_HEADERS
    formats/format_utils.h
    formats/gguf_tokenizer_reader.h
    formats/hf_tokenizer_reader.h
    formats/binary_vocab_reader.h
)

set(JOBTOKEN_FORMATS_SOURCES
    formats/hf_tokenizer_reader.cpp
    formats/binary_vocab_reader.cpp
    formats/gguf_tokenizer_reader.cpp
)

if(JOB_LINUX)
    list(APPEND JOBTOKEN_FORMATS_HEADERS formats/tokenizer_shm.h)
    list(APPEND JOBTOKEN_FORMATS_SOURCES formats/tokenizer_shm.cpp)
endif()



set(JOBTOKEN_TEMPLATE_HEADERS
    template/chat_template_engine.h
    template/chat_message.h
    template/jinja_utils.h
    template/jinja_token.h
    template/jinja_lexer.h
    template/jinja_ast.h
    template/jinja_parser.h
    template/jinja_vm.h
)

set(JOBTOKEN_TEMPLATE_SOURCES
    template/chat_template_engine.cpp
    template/jinja_lexer.cpp
    template/jinja_parser.cpp
    template/jinja_vm.cpp
)

set(JOBTOKEN_VOCAB_HEADERS
    vocab/token_record.h
    vocab/vocab.h
    vocab/special_tokens.h
)

set(JOBTOKEN_VOCAB_SOURCES

)

set(JOBTOKEN_COMPILE_THIS
    ${JOBTOKEN_HEADERS}            ${JOBTOKEN_SOURCES}
    ${JOBTOKEN_ALGO_HEADERS}       ${JOBTOKEN_ALGO_SOURCES}
    ${JOBTOKEN_CORE_HEADERS}       ${JOBTOKEN_CORE_SOURCES}
    ${JOBTOKEN_FORMATS_HEADERS}    ${JOBTOKEN_FORMATS_SOURCES}
    ${JOBTOKEN_TEMPLATE_HEADERS}   ${JOBTOKEN_TEMPLATE_SOURCES}
    ${JOBTOKEN_VOCAB_HEADERS}      ${JOBTOKEN_VOCAB_SOURCES}
)

if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Token SHARED ${JOBTOKEN_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Token STATIC ${JOBTOKEN_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Token PUBLIC JOBTOKEN_STATIC_DEFINE)
endif()



set(JOBTOKEN_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/jobtoken_export.h")
generate_export_header(JosephsOddBuilder_Token
    BASE_NAME JOBTOKEN
    EXPORT_FILE_NAME "${JOBTOKEN_EXPORT_HEADER}"
)

set_target_properties(JosephsOddBuilder_Token PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobtoken"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Token PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/algo>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/algo>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/core>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/formats>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/formats>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/template>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/template>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vocab>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/vocab>

    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

target_link_libraries(JosephsOddBuilder_Token PRIVATE
    JosephsOddBuilder_Core
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ggml
    Threads::Threads
    $<BUILD_INTERFACE:ggml>
)

if(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Token PUBLIC atomic)
    target_link_libraries(JosephsOddBuilder_Token PRIVATE JosephsOddBuilder_IO)
elseif(JOB_WINDOWS)
    target_compile_options(JosephsOddBuilder_Token PRIVATE /wd4251)
endif()


install(TARGETS JosephsOddBuilder_Token
    EXPORT JosephsOddBuilderTokenTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken
)

install(FILES ${JOBTOKEN_HEADERS}               DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken)
install(FILES ${JOBTOKEN_EXPORT_HEADER}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken)
install(FILES ${JOBTOKEN_ALGO_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/algo)
install(FILES ${JOBTOKEN_CORE_HEADERS}         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/core)
install(FILES ${JOBTOKEN_FORMATS_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/formats)
install(FILES ${JOBTOKEN_TEMPLATE_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/template)
install(FILES ${JOBTOKEN_VOCAB_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/vocab)

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobtoken.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobtoken.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )

    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}" )
endif()

install(EXPORT JosephsOddBuilderTokenTargets
    FILE JosephsOddBuilderTokenTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderToken"
)

