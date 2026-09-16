if(JOB_BUILD_SERIALIZER_MSGPACK)
    add_subdirectory(job_msg_gen_example)
endif()

if(JOB_BUILD_ANSI_LEGACY AND JOB_BUILD_TUI)
    add_subdirectory(job_ansi_examples)
    if(JOB_BUILD_AI)
        add_subdirectory(job_ai_terminal_dashboard_example)
    endif()
endif()

# add_subdirectory(job-reflection-test) go away you are almost in core ...

if(JOB_BUILD_MODEL AND JOB_BUILD_GGML AND JOB_BUILD_TOKEN AND JOB_CUDA)
    add_subdirectory(job-inference-test)
endif()