if(JOB_BUILD_SERIALIZER_MSGPACK)
    add_subdirectory(job_msg_gen)
    add_subdirectory(job_msg_gen_example)
endif()

if(JOB_BUILD_ANSI AND JOB_BUILD_TUI)
    add_subdirectory(job_ansi_examples)
    if(JOB_BUILD_AI)
        add_subdirectory(job_ai_terminal_dashboard_example)
    endif()
endif()

add_subdirectory(job-reflection-test)

if(JOB_QT)

    if(JOB_BUILD_SCIENCE)
        add_subdirectory(qt_protoplanetary_view)
    endif()

    if(JOB_BUILD_ZSTD AND JOB_BUILD_QZSTD)
        add_subdirectory(qzstd-example)
        add_subdirectory(qmlzstd-example)
    endif()


    if(JOB_BUILD_QHF)
        # add_subdirectory(qmlhf-example)
    endif()

    if(JOB_BUILD_CRYPTO AND JOB_BUILD_QSODIUM)
        add_subdirectory(qsodium-example)
        add_subdirectory(qmlsodium-example)
    endif()

    if(JOB_BUILD_QLLAMA)
        add_subdirectory(qllama-simple-example)
        add_subdirectory(qllama-qml-client-server-example)
    endif()

    if(JOB_BUILD_QSD)
        add_subdirectory(qmlsd-example)
    endif()
endif()



# add_subdirectory(job-inference-test)
