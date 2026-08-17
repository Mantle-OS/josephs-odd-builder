add_subdirectory(job_simd)
add_subdirectory(job_core)
add_subdirectory(job_threads)
add_subdirectory(job_crypto)
add_subdirectory(job_zstd)

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

add_subdirectory(job_ggml)
add_subdirectory(job_token)
add_subdirectory(job_model)

add_subdirectory(job_sound)





if(JOB_QT)
    if(JOB_BUILD_SERIALIZER_MSGPACK)
        add_subdirectory(aipkg-schema)      ## this generated code needs to have the whole dll exports ....
        add_subdirectory(job_aipkg)         ## not cross platform yet
    endif()

    add_subdirectory(qaiutils)              # fully cross platform | not tested on hardware for win32
    add_subdirectory(qsodium)               # fully cross platform | not tested on hardware for win32
    add_subdirectory(qzstd)                 # fully cross platform | not tested on hardware for win32

    add_subdirectory(qsd)                   # fully cross platform | not tested on hardware for win32
    # add_subdirectory(qllama)                # fully cross platform | not tested on hardware for win32 | NOTE: I got towards the end of this and only exposed (get it working now deal) so much. This should be refactored to soon to "really" expose the whole lamma_cpp deal.

    # These are 99% done need package manager so that the session manager has that.
    # once the session manager has that the stack is 100% free.
    # the package manager is blocked by my (JobNet)Networking port.
    # add_subdirectory(qhf)                 # Depends on session manager
    # add_subdirectory(qsessionmanager)     # Depends on qaipkg which depends on job_aipkg which depends on job_net

    # add_subdirectory(qggml) # just me playing around trying to learn the whole device, graph, weights and models
endif()
