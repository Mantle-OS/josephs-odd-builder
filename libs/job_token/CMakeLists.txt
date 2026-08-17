project(JosephsOddBuilder_Token
    VERSION 0.1
    LANGUAGES CXX
)

set(JOBTOKEN_HEADERS
    job_token_enums.h
    job_token_types.h # see needs enum file / class's
    # job_tokenizer_error.h # see needs enum file / class's DELETE
    job_token.h  # the factory manager and other facade deals to make the library code not leak
)

set(JOBTOKEN_SOURCES
    job_token.cpp
)

set(JOBTOKEN_ALGO_HEADERS
    algo/itoken_algo.h
    algo/bpe.h
    algo/unigram.h
    algo/wordpiece.h
    algo/token_algo_factory.h
)

set(JOBTOKEN_ALGO_SOURCES
    algo/bpe.cpp
    algo/unigram.cpp
    algo/wordpiece.cpp
    algo/token_algo_factory.cpp
)

set(JOBTOKEN_CORE_HEADERS
    core/trie.h
    core/unicode_normalizer.h
    core/regex_splitter.h
    core/byte_fallback.h
    # core/byte_level.h
)

set(JOBTOKEN_CORE_SOURCES
    core/regex_splitter.cpp
    core/unicode_normalizer.cpp
    # core/byte_level.cpp
)

set(JOBTOKEN_TOKEN_HEADERS
    token/itoken.h
    token/gguf_token.h
    token/hf_token.h
    token/binary_token.h
    token/token_factory.h
)

set(JOBTOKEN_TOKEN_SOURCES
    token/hf_token.cpp
    token/binary_token.cpp
    token/gguf_token.cpp
    token/token_factory.cpp
)

## the first transport layer could live in a differnt folder but there are enough folders at moment
## install section and JOBTOKEN_COMPILE_THIS proves this
if(JOB_LINUX)
    list(APPEND JOBTOKEN_IPC_HEADERS ipc/token_shm.h)
    list(APPEND JOBTOKEN_IPC_SOURCES ipc/token_shm.cpp)
endif()


## optinal chat templete so we dont have to pass in |imuser| or whatever...
set(JOBTOKEN_CHAT_HEADERS
    chat/chat_engine.h
    chat/chat_message.h
    # chat/jinja_utils.h
    chat/jinja_token.h
    chat/jinja_lexer.h
    chat/jinja_ast.h
    chat/jinja_parser.h
    chat/jinja_vm.h
)

set(JOBTOKEN_CHAT_SOURCES
    chat/chat_engine.cpp
    chat/jinja_lexer.cpp
    chat/jinja_parser.cpp
    chat/jinja_vm.cpp
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
    ${JOBTOKEN_TOKEN_HEADERS}      ${JOBTOKEN_TOKEN_SOURCES}
    ${JOBTOKEN_IPC_HEADERS}        ${JOBTOKEN_IPC_SOURCES}
    ${JOBTOKEN_CHAT_HEADERS}       ${JOBTOKEN_CHAT_SOURCES}
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

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/token>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/token>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ipc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/ipc>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/chat>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/chat>

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

install(FILES ${JOBTOKEN_HEADERS}           DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken)
install(FILES ${JOBTOKEN_EXPORT_HEADER}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken)
install(FILES ${JOBTOKEN_ALGO_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/algo)
install(FILES ${JOBTOKEN_CORE_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/core)
install(FILES ${JOBTOKEN_TOKEN_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/token)
install(FILES ${JOBTOKEN_IPC_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/ipc)
install(FILES ${JOBTOKEN_CHAT_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/chat)
install(FILES ${JOBTOKEN_VOCAB_HEADERS}     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobtoken/vocab)

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

