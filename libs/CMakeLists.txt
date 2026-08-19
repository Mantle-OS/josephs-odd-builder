
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


if(JOB_CUDA)
    # add_subdirectory(job_cuda)  ## OLD me learning cuda can be deleted
    if(JOB_BUILD_GGML)
        add_subdirectory(job_ggml)

        if(JOB_BUILD_MODEL)
            add_subdirectory(job_model)
        endif()
        if(JOB_BUILD_TOKEN)
            add_subdirectory(job_token)
        endif()
    endif()
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
        if(JOB_BUILD_AIPKG_SCHEMA)
            add_subdirectory(aipkg-schema)

            if(JOB_BUILD_AIPKG)
                add_subdirectory(job_aipkg)
            endif()

        endif()
    endif()



endif()

if(JOB_BUILD_SCIENCE)
    add_subdirectory(job_science)
endif()

if(JOB_BUILD_AI)
    add_subdirectory(job_ai)
endif()


if(JOB_BUILD_ANSI)
    add_subdirectory(job_ansi)
endif()

if(JOB_BUILD_TUI)
    add_subdirectory(job_tui)
endif()


if(JOB_BUILD_SOUND)
    add_subdirectory(job_sound)
endif()



if(JOB_QT)

    if(JOB_BUILD_QAIUTILS)
        add_subdirectory(qaiutils)
    endif()

    if(JOB_BUILD_CRYPTO AND JOB_BUILD_QSODIUM)
        add_subdirectory(qsodium)
    endif()
    add_subdirectory(qzstd)

    if(JOB_BUILD_QSD)
        add_subdirectory(qsd)                   # fully cross platform | not tested on hardware for win32
    endif()



    if(JOB_BUILD_QLLAMA)
        add_subdirectory(qllama)                # fully cross platform | not tested on hardware for win32 | NOTE: I got towards the end of this and only exposed (get it working now deal) so much. This should be refactored to soon to "really" expose the whole lamma_cpp deal.
    endif()



    if(JOB_BUILD_QHF)
        # add_subdirectory(qhf)                 # Depends on session manager
    endif()

    if(JOB_BUILD_QSESSIONMANAGER)
        # add_subdirectory(qsessionmanager)     # Depends on qaipkg which depends on job_aipkg which depends on job_net
    endif()
endif()
