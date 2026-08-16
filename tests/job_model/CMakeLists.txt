cmake_minimum_required(VERSION 3.16)

# ../transient_test_file.h

add_executable(test_job_model
    main.cpp
    test_model_config.cpp
    test_kv_cache.cpp
    test_compute_graph_builder.cpp
    test_weights.cpp
    test_model.cpp
)
target_compile_definitions(test_job_model PRIVATE
    JOB_TEST_GGUF_FILE="/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
)

target_compile_definitions(test_job_model PRIVATE
    JOB_MODEL_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/hf/Qwen/Qwen3-4B-Instruct-2507"
)

target_link_libraries(test_job_model PRIVATE
    JosephsOddBuilder_Core
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ggml
    JosephsOddBuilder_Token
    JosephsOddBuilder_Model
    Threads::Threads
    $<BUILD_INTERFACE:ggml>
    ${CatchTwo_LIBRARIES}
)

if(JOB_LINUX)
    target_link_libraries(test_job_model PUBLIC atomic)
    target_link_libraries(test_job_model PRIVATE JosephsOddBuilder_IO)
elseif(JOB_WINDOWS)
    target_compile_options(test_job_model PRIVATE /wd4251)
endif()

target_include_directories(test_job_model PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_model COMMAND test_job_model)
