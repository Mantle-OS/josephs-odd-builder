add_subdirectory(job_core)                  # fully cross platform | not tested on hardware for win32
add_subdirectory(job_threads)               # fully cross platform | not tested on hardware for win32
add_subdirectory(job_crypto)                # fully cross platform | not tested on hardware for win32
add_subdirectory(job_zstd)                  # fully cross platform | not tested on hardware for win32
add_subdirectory(job_simd)                  # not cross platform yet
if(JOB_CUDA)
    add_subdirectory(job_cuda)
endif()
add_subdirectory(job_io)                    # not cross platform yet JobSharedMNemory and the Pty parts are gonna be tough esp tyhe shared memory
add_subdirectory(job_uart)                  # not cross platform currently uses udev for monotor of serial bus | look into what windows has for this.... "Here is looking at you putty"
add_subdirectory(job_net)                 ## where the new network code
add_subdirectory(job_serializer)            # fully cross platform but needs to generate and export the stupid dll exports

add_subdirectory(job_science)               # not cross platform yet

add_subdirectory(job_ai)                    # not cross platform yet
add_subdirectory(job_ansi)                  # not cross platform  This one is gonna be pretty easy as long as windows talks Ansii lol
add_subdirectory(job_tui)                   # see above

# add_subdirectory(job_ggml)                ## old going away was just me playing around

if(JOB_QT)
    if(JOB_BUILD_SERIALIZER_MSGPACK)
        add_subdirectory(aipkg-schema)      ## this generated code needs to have the whole dll exports ....
        add_subdirectory(job_aipkg)         ## not cross platform yet
    endif()

    add_subdirectory(qaiutils)              # fully cross platform | not tested on hardware for win32
    add_subdirectory(qsodium)               # fully cross platform | not tested on hardware for win32
    add_subdirectory(qzstd)                 # fully cross platform | not tested on hardware for win32

    # These are 99% done need package manager so that the session manager has that.
    # once the session manager has that the stack is 100% free.
    # the package manager is blocked by my (JobNet)Networking port.

    # add_subdirectory(qsd)                 # Depends on session manager
    # add_subdirectory(qllama)              # Depends on session manager
    # add_subdirectory(qhf)                 # Depends on session manager
    # add_subdirectory(qsessionmanager)     # Depends on qaipkg which depends on job_aipkg which depends on job_net


    # add_subdirectory(qggml) # was just for playing around with will be deleted
endif()
