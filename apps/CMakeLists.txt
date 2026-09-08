if(JOB_BUILD_SERIALIZER_MSGPACK)
    add_subdirectory(job_msg_gen_example)
endif()

if(JOB_BUILD_ANSI AND JOB_BUILD_TUI)
    add_subdirectory(job_ansi_examples)
    if(JOB_BUILD_AI)
        add_subdirectory(job_ai_terminal_dashboard_example)
    endif()
endif()

# add_subdirectory(job-reflection-test)
add_subdirectory(job-inference-test)
