add_subdirectory(job_core)
add_subdirectory(job_threads)
add_subdirectory(job_crypto)
add_subdirectory(job_zstd)
add_subdirectory(job_simd)
if(JOB_CUDA)
    add_subdirectory(job_cuda)
endif()
add_subdirectory(job_io)
add_subdirectory(job_uart)
add_subdirectory(job_net)
add_subdirectory(job_serializer)
add_subdirectory(job_science)
add_subdirectory(job_ai)
add_subdirectory(job_ansi)
add_subdirectory(job_tui)
# add_subdirectory(job_ggml)

if(JOB_QT_AI)

    if(JOB_SER_MSGPACK_BINDINGS)
        add_subdirectory(aipkg-schema)
    endif()

    add_subdirectory(qaiutils)
    add_subdirectory(qzstd)
    add_subdirectory(qhf)

    add_subdirectory(qsessionmanager)

    add_subdirectory(qllama)
    # add_subdirectory(qggml)
    add_subdirectory(qsodium)


    add_subdirectory(qsd)
endif()
