cmake_minimum_required(VERSION 3.16)

set(JOBTOKEN_TESTS
    # test_jinja_lexer.cpp
    # test_jinja_parser.cpp
    # test_jinja_vm.cpp
    # test_chat_template_engine.cpp
    # test_hf_tokenizer_reader.cpp
    # test_binary_vocab_reader.cpp
    # test_gguf_tokenizer_reader.cpp
    # test_job_tokenizer.cpp

)

# list(APPEND JOBTOKEN_TESTS test_tokenizer_shm.cpp)
if(JOB_LINUX)
endif()

add_executable(test_job_token
    main.cpp
    # ../transient_test_file.h
    # ${JOBTOKEN_TESTS}
)

target_compile_definitions(test_job_token PRIVATE
    JOB_TOKEN_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/hf"
)

target_link_libraries(test_job_token PRIVATE
    JosephsOddBuilder_Core
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ggml
    JosephsOddBuilder_Token
    Threads::Threads
    $<BUILD_INTERFACE:ggml>
    ${CatchTwo_LIBRARIES}
)

if(JOB_LINUX)
    target_link_libraries(test_job_token PUBLIC atomic)
    target_link_libraries(test_job_token PRIVATE JosephsOddBuilder_IO)
elseif(JOB_WINDOWS)
    target_compile_options(test_job_token PRIVATE /wd4251)
endif()

target_include_directories(test_job_token PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_token COMMAND test_job_token)
