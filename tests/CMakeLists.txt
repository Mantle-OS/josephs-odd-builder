set(TEST_SPIN_TILL test_spin_till.h )
add_subdirectory(job_core)
add_subdirectory(job_threads)
add_subdirectory(job_crypto)
add_subdirectory(job_io)
add_subdirectory(job_uart)
add_subdirectory(job_net)
add_subdirectory(job_serializer)
if(MSGPACK_BINDINGS)
    add_subdirectory(job_serializer_msgpack)
endif()

# CHECKPOINT: v1.0
