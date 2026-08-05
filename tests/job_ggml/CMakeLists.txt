cmake_minimum_required(VERSION 3.16)


add_executable(test_job_ggml
    ../transient_test_file.h
    main.cpp                            # 1) NOTE this is special because you guessed it ... Thanks Nvidia DONE
    test_ggml_utils.h                   # 2) singleton and utils for the test harness DONE
    test_ggml_device_manager.cpp        # 3) discovers devices and constructs schedulers DONE
    test_ggml_backend_sched.cpp         # 4) assigns, allocates, and executes tensor graphs DONE
    test_ggml_context_tensor.cpp        # 5) context ownership and tensor inspection/access DONE
    test_gguf.cpp                       # 6) test the whole gguf framework lol

    test_gguf_context.cpp         # context inspection and mutation
    test_gguf_kv.cpp              # every scalar/array type and invalid access
    test_gguf_type_traits.cpp     # compile-time and runtime mappings
    # test_gguf_tensor_info.cpp     # copied tensor metadata and offsets
)
# test_ggml_training.cpp              # 6) forward/backward and optimizer behavior

target_compile_definitions(test_job_ggml PRIVATE
    JOB_TEST_GGUF_FILE="/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
)

if(JOB_WINDOWS)
    target_compile_options(test_job_ggml PRIVATE /wd4251)
elseif (JOB_LINUX)
    target_link_libraries(test_job_ggml PRIVATE atomic)
endif()

target_link_libraries(test_job_ggml PRIVATE
    ${CatchTwo_LIBRARIES}
    Threads::Threads
    $<BUILD_INTERFACE:ggml>
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ggml
)

target_include_directories(test_job_ggml PRIVATE
    ${CatchTwo_INCLUDE_DIRS}

)

add_test(NAME job_ggml COMMAND test_job_ggml)
