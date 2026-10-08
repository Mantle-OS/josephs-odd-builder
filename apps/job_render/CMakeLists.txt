if(JOB_BUILD_RENDER_FB)
    add_subdirectory(job-fb-basic-test)
endif()

if(HAS_SIMPLE_KMS)
    add_subdirectory(job-kms-simple-basic-test)
endif()
if(HAS_FULL_KMS)
    add_subdirectory(job-kms-basic-test)
    add_subdirectory(job-kms-damage-quadrant-test)



    # the final boss lol
    # add_subdirectory(job-kms-damage-test)
endif()

