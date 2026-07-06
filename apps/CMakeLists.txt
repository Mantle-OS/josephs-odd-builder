if(JOB_SER_MSGPACK_BINDINGS)
    add_subdirectory(job_msg_gen)
    add_subdirectory(job_msg_gen_example)
endif()

add_subdirectory(job_ansi_examples)
add_subdirectory(job_ai_terminal_dashboard_example)

if(JOB_QT_APPS)
    add_subdirectory(qt_protoplanetary_view)
endif()

if(JOB_QT_AI)
    add_subdirectory(qzstd-example)
    add_subdirectory(qmlzstd-example)

    # add_subdirectory(qmlsd-example)
    # add_subdirectory(qmlhf-example)

    add_subdirectory(qsodium-example)
    add_subdirectory(qmlsodium-example)

    # add_subdirectory(qllama-simple-example)
    # add_subdirectory(qllama-qml-client-server-example)
endif()
