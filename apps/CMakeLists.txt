if(MSGPACK_BINDINGS)
    add_subdirectory(job_msg_gen)
    add_subdirectory(job_msg_gen_example)
endif()

add_subdirectory(job_ansi_examples)
add_subdirectory(job_ai_terminal_dashboard_example)

if(JOB_QT_APPS)
    add_subdirectory(qt_protoplanetary_view)
endif()
