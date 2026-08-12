cmake_minimum_required(VERSION 3.16)

set(JOB_GGML_TEST_IMPL test_ggml_cpu.cpp)
if(GGML_VULKAN)
    add_compile_definitions(JOB_GGML_VULKAN)
    list(APPEND JOB_GGML_TEST_IMPL test_ggml_vulkan.cpp)
endif()

if(GGML_CUDA)
    add_compile_definitions(JOB_GGML_CUDA)
    list(APPEND JOB_GGML_TEST_IMPL test_ggml_cuda.cpp)
endif()

add_compile_definitions(JOB_GGML_OPENCL)
add_compile_definitions(JOB_GGML_BLAS)

## That is good enough for the devices due to hardware limitation

add_executable(test_job_ggml
    ../transient_test_file.h
    main.cpp                            # 1) NOTE this is special because you guessed it ... Thanks Nvidia
    test_ggml_utils.h                   # 2) singleton and utils for the test harness
    test_ggml_device_manager.cpp        # 3) discovers devices and constructs schedulers
    test_ggml_backend_sched.cpp         # 4) assigns, allocates, and executes tensor graphs
    test_ggml_context_tensor.cpp        # 5) context ownership and tensor inspection/access
    test_gguf.cpp                       # 6) test the whole gguf framework
    test_gguf_context.cpp               # 7) context inspection and mutation
    test_gguf_kv.cpp                    # 8) every scalar/array type and invalid access
    test_gguf_type_traits.cpp           # 9) compile-time and runtime mappings
    test_gguf_tensor_info.cpp           # 10) copied tensor metadata and offsets
    test_ggml_opt_params.cpp            # 11) optimizer parameters, schedules, and native conversion
    test_ggml_opt_dataset_result.cpp    # 12) dataset storage/batching and accumulated results
    test_ggml_opt_context.cpp           # 13) optimizer context ownership, tensors, allocation, and reset
    test_ggml_opt_training.cpp          # 14) epoch/fit callbacks and real backward parameter updates
    test_ggml_tensor_op_unary.cpp       # 15.a) unary operations
    test_ggml_tensor_op_multi.cpp       # 15.b) binary and multi-tensor operations
    test_ggml_tensor_op_transform.cpp   # 15.c) parameterized transformations and views
    test_ggml_tensor_op_special.cpp     # 15.d) RoPE, attention, convolution, pooling, SSM, etc.
    test_ggml_tensor_op_custom.cpp      # 15.e) custom callbacks and raw/custom operations
    test_ggml_tensor_op_graph.cpp       # 16) expression DAG construction and forward graph expansion
    test_ggml_quant.cpp
    test_ggml.cpp
    # ${JOB_GGML_TEST_IMPL}               # the devices special tests
)

# test_ggml_tensor_op_for_science.cpp # fun side quest

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
    ${JOB_CUDA_LIBS}

    ## just to mess around
    # JosephsOddBuilder_Science
    # JosephsOddBuilder_Ai
)

target_include_directories(test_job_ggml PRIVATE
    ${CatchTwo_INCLUDE_DIRS}

)

add_test(NAME job_ggml COMMAND test_job_ggml)
