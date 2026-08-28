if(JOB_BUILD_SIMD)
    add_subdirectory(job_simd)
endif()

if(JOB_BUILD_CORE)
    add_subdirectory(job_core)
endif()

if(JOB_BUILD_THREADS)
    add_subdirectory(job_threads)
endif()

if(JOB_BUILD_CRYPTO)
    add_subdirectory(job_crypto)
endif()


if(JOB_BUILD_ZSTD)
    add_subdirectory(job_zstd)
endif()

if(JOB_LINUX)
    if(JOB_BUILD_IO)
        add_subdirectory(job_io)
    endif()
    if(JOB_BUILD_UART)
        add_subdirectory(job_uart)
    endif()
endif()


if(JOB_BUILD_NET)
    add_subdirectory(job_net)
endif()

if(JOB_BUILD_SERIALIZER)
    add_subdirectory(job_serializer)
    if(JOB_BUILD_SERIALIZER_MSGPACK)
        add_subdirectory(job_serializer_msgpack)
        if(JOB_BUILD_AIPKG)
            add_subdirectory(job_aipkg)
        endif()
    endif()
endif()

if(JOB_BUILD_ANSI)
    add_subdirectory(job_ansi)
endif()

if(JOB_BUILD_SCIENCE)
    add_subdirectory(job_science)
endif()

if(JOB_BUILD_AI)
    add_subdirectory(job_ai)
endif()

if(JOB_BUILD_SOUND)
    add_subdirectory(job_sound)
endif()

if(JOB_CUDA)
    if(JOB_BUILD_GGML)
       add_subdirectory(job_ggml)
    endif()

    if(JOB_BUILD_TOKEN)
        add_subdirectory(job_token)
    endif()

    if(JOB_BUILD_MODEL)
        # add_subdirectory(job_model)
    endif()
endif()
