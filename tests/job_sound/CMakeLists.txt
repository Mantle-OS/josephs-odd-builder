cmake_minimum_required(VERSION 3.16)
# ../test_spin_till.h ## if needed
add_executable(test_job_sound
    main.cpp
    test_dsp.cpp
    test_models.cpp
    test_alsa.cpp
    test_pipewire.cpp
    test_audio_pipeline.cpp
    test_codecs.cpp
    ../transient_test_file.h
)
target_compile_options(test_job_sound PRIVATE -freflection -fcontracts)
target_link_options(test_job_sound PRIVATE -fcontracts)

set_target_properties(test_job_sound PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(test_job_sound PRIVATE
    Threads::Threads
    atomic
    ${CatchTwo_LIBRARIES}
    JosephsOddBuilder_Threads
    JosephsOddBuilder_Core
    JosephsOddBuilder_Sound

    ${LibAlsa_LIBRARIES}
    ${LibSPA_LIBRARIES}
    ${LibPipewire_LIBRARIES}
    ${LibWirePlumber_LIBRARIES}
    ${LibAlsa_LIBRARIES}
    ${LibSPA_LIBRARIES}
    ${LibPipewire_LIBRARIES}
    ${LibWirePlumber_LIBRARIES}
    ${LibOpus_LIBRARIES}
    ${LibFLAC_LIBRARIES}
    ${LibOgg_LIBRARIES}
    ${LibVorbis_LIBRARIES}
    ${LibWavPack_LIBRARIES}
)

target_include_directories(test_job_sound PRIVATE
    ${CatchTwo_INCLUDE_DIRS}
)

add_test(NAME job_sound COMMAND test_job_sound)
