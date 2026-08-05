add_subdirectory(job_core)                  # fully cross platform | not tested on hardware for win32
add_subdirectory(job_threads)               # fully cross platform | not tested on hardware for win32
add_subdirectory(job_crypto)                # fully cross platform | not tested on hardware for win32
add_subdirectory(job_zstd)                  # fully cross platform | not tested on hardware for win32
add_subdirectory(job_simd)                  # fully cross platform | not tested on hardware for win32

if(JOB_CUDA)
    add_subdirectory(job_cuda)              # This was just me playing around with cuda to see if I could get my cuda_bridge and a gemm that I could test against of job_ai benchmarks
endif()

add_subdirectory(job_io)                    # not cross platform yet JobSharedMNemory and the Pty parts are gonna be tough esp tyhe shared memory
add_subdirectory(job_uart)                  # not cross platform currently uses udev for monotor of serial bus | look into what windows has for this.... "Here is looking at you putty"
add_subdirectory(job_net)                   # SEMI cross platform. | not tested on hardware for win32

add_subdirectory(job_serializer)            # fully cross platform but needs to generate and export the stupid dll exports

add_subdirectory(job_science)               # not cross platform yet | will not be that hard to port
add_subdirectory(job_ai)                    # not cross platform yet | will not port the whole goal of this library was to see if science and AI algoritums would prove out to work on a very limited(NO GPU, TPU, ETC) hardware rig think less than a gig of ram on a SOM
add_subdirectory(job_ansi)                  # not cross platform     | This one is gonna be pretty easy as long as windows talks Ansii lol
add_subdirectory(job_tui)                   # see above as it depends on the whole ansi protocol

add_subdirectory(job_ggml)                ## old going away was just me playing around trying to learn the whole device, graph, weights and models

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
