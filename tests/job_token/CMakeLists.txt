cmake_minimum_required(VERSION 3.16)

set(JOBTOKEN_TESTS
    test_vocab.cpp
    test_special_tokens.cpp
    test_trie.cpp
    test_byte_fallback.cpp
    test_unicode_normalizer.cpp
    test_regex_splitter.cpp

    test_bpe.cpp
    test_unigram.cpp
    test_wordpiece.cpp
    test_token_algo_factory.cpp

    test_hf_token.cpp
    test_gguf_token.cpp
    test_binary_token.cpp
    test_token_factory.cpp

    test_chat_engine.cpp
    test_jinja_lexer.cpp
    test_jinja_parser.cpp
    test_jinja_vm.cpp

    test_job_token.cpp
    test_tokenizer_shm.cpp
)

add_executable(test_job_token
    main.cpp
    ../transient_test_file.h
    ${JOBTOKEN_TESTS}
)

if(NOT JOB_CI_BUILD)
    target_compile_definitions(test_job_token PRIVATE
        JOB_TOKEN_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/hf"
    )

    target_compile_definitions(test_job_token PRIVATE
        JOB_TOKEN_TEST_GGUF_FILE="/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
    )
endif()
target_compile_options(test_job_token PRIVATE -freflection -fcontracts)
target_link_options(test_job_token PRIVATE -fcontracts)

set_target_properties(test_job_token PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(test_job_token
    PUBLIC
      atomic
    PRIVATE
      JosephsOddBuilder_Core
      JosephsOddBuilder_Threads
      JosephsOddBuilder_IO
      JosephsOddBuilder_Ggml
      JosephsOddBuilder_Token
      Threads::Threads
      $<BUILD_INTERFACE:ggml>
      ${CatchTwo_LIBRARIES}
)

target_include_directories(test_job_token PRIVATE ${CatchTwo_INCLUDE_DIRS})
add_test(NAME job_token COMMAND test_job_token)
