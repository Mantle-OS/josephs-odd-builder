cmake_minimum_required(VERSION 3.16)

add_executable(test_job_ggml
    main.cpp
    test_ggml_utils.h   # maybe singleton to device manager
    # test_ggml_graph.cpp # pass
    # test_ggml_device.cpp  # fails
    test_ggml_device_manager.cpp # fails
    # test_ggml_weights.cpp  # empty
    # test_ggml_model.cpp    # fails need to rewrite and get a guf file to test against
)

target_link_libraries(test_job_ggml PRIVATE
    ${CatchTwo_LIBRARIES}
    ${LibGgml_LIBRARIES}
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Ggml
)

target_include_directories(test_job_ggml PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
    ${LibGgml_INCLUDE_DIRS}
)

add_test(NAME job_ggml COMMAND test_job_ggml)
