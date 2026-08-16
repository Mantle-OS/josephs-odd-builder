project(JosephsOddBuilder_Sound
    VERSION 0.1
    LANGUAGES CXX C
)

include_directories(${LibAlsa_INCLUDE_DIRS})
include_directories(${LibPipewire_INCLUDE_DIRS})
include_directories(${LibSPA_INCLUDE_DIRS})
include_directories(${LibWirePlumber_INCLUDE_DIRS})

set(JOBSOUND_BACKEND_ALSA_HEADERS
    backends/alsa/alsa_playback_thread.h
    backends/alsa/alsa_sound_card.h
    backends/alsa/alsa_control.h
    backends/alsa/alsa_device.h
    backends/alsa/alsa_capture_thread.h
    backends/alsa/alsa_mixer.h
    backends/alsa/alsa_manager.h
)

set(JOBSOUND_BACKEND_PIPEWIRE_HEADERS
    backends/pipewire/pipewire_tone_source.h
    backends/pipewire/pipewire_graph_node.h
    backends/pipewire/pipewire_graph_adapter.h
    backends/pipewire/pipewire_stream.h
    backends/pipewire/pipewire_device.h
    backends/pipewire/pipewire_manager.h

    backends/pipewire/pipewire_port.h
    backends/pipewire/pipewire_link.h
    backends/pipewire/pipewire_graph_linker.h
    backends/pipewire/pipewire_link_layer.h
)

set(JOBSOUND_DSP_HEADERS
    dsp/virtual_eq_band.h
    dsp/equalizerbank.h
    dsp/biquad_eq_31band.h
    dsp/virtual_eq_processor.h
    dsp/fft_analyzer.h
    dsp/virtual_mixer_channel.h

    dsp/audio_effects.h
    dsp/overdrive_effect.h
    dsp/flanger_effect.h
    dsp/chorus_effect.h
    dsp/delay_effect.h
    dsp/compressor_effect.h
    dsp/reverb_effect.h
    dsp/tremolo_effect.h
    dsp/bitcrusher_effect.h
    dsp/phaser_effect.h
    dsp/noise_gate_effect.h
    dsp/envelope_filter_effect.h
    dsp/wah_effect.h
    dsp/ring_modulator_effect.h
    dsp/audio_effect_factory.h


    dsp/dsp_manager.h
)

set(JOBSOUND_MIDI_HEADERS
    # midi/midi_router.h
    # midi/midi_manager.h
    # midi/midi_event.h
)

set(JOBSOUND_CODECS_HEADERS
    codecs/audio_codec_params.h
    codecs/audio_decoder.h
    codecs/audio_encoder.h
    codecs/audio_packet.h
)

set(JOBSOUND_CODECS_PCM_HEADERS
    codecs/pcm/wav_reader.h
    codecs/pcm/wav_writer.h
)

set(JOBSOUND_CODECS_OPUS_HEADERS
    codecs/opus/opus_decoder.h
    codecs/opus/opus_encoder.h
)

set(JOBSOUND_CODECS_FLAC_HEADERS
    codecs/flac/flac_decoder.h
    codecs/flac/flac_encoder.h
)

set(JOBSOUND_BACKEND_ALSA_SOURCES
    backends/alsa/alsa_manager.cpp
    backends/alsa/alsa_playback_thread.cpp
    backends/alsa/alsa_sound_card.cpp
    backends/alsa/alsa_control.cpp
    backends/alsa/alsa_device.cpp
    backends/alsa/alsa_capture_thread.cpp
    backends/alsa/alsa-mixer.cpp
)

set(JOBSOUND_BACKEND_PIPEWIRE_SOURCES
    backends/pipewire/pipewire_tone_source.cpp
    backends/pipewire/pipewire_stream.cpp
    backends/pipewire/pipewire_graph_node.cpp

    backends/pipewire/pipewire_link_layer.cpp
    backends/pipewire/pipewire_graph_adapter.cpp
)


set(JOBSOUND_DSP_SOURCES
    dsp/audio_effect_factory.cpp
    dsp/dsp_manager.cpp
)

set(JOBSOUND_MIDI_SOURCES
    # midi/midi_router.cpp
    # midi/midi_manager.cpp
    # midi/midi_event.cpp
)

set(JOBSOUND_CODECS_SOURCES


)

set(JOBSOUND_CODECS_PCM_SOURCES
    codecs/pcm/wav_reader.cpp
    codecs/pcm/wav_writer.cpp
)

set(JOBSOUND_CODECS_OPUS_SOURCES
    codecs/opus/opus_decoder.cpp
    codecs/opus/opus_encoder.cpp
)

set(JOBSOUND_CODECS_FLAC_SOURCES
    codecs/flac/flac_decoder.cpp
    codecs/flac/flac_encoder.cpp
)



set(JOBSOUND_COMPILE_THIS
    ${JOBSOUND_BACKEND_ALSA_HEADERS}        ${JOBSOUND_BACKEND_ALSA_SOURCES}
    ${JOBSOUND_BACKEND_PIPEWIRE_HEADERS}    ${JOBSOUND_BACKEND_PIPEWIRE_SOURCES}
    ${JOBSOUND_DSP_HEADERS}                 ${JOBSOUND_DSP_SOURCES}
    # ${JOBSOUND_MIDI_HEADERS}                ${JOBSOUND_MIDI_SOURCES}
    ${JOBSOUND_CODECS_HEADERS}              ${JOBSOUND_CODECS_SOURCES}
    ${JOBSOUND_CODECS_PCM_HEADERS}          ${JOBSOUND_CODECS_PCM_SOURCES}
    ${JOBSOUND_CODECS_OPUS_HEADERS}         ${JOBSOUND_CODECS_OPUS_SOURCES}
    ${JOBSOUND_CODECS_FLAC_HEADERS}         ${JOBSOUND_CODECS_FLAC_SOURCES}
)


if(BUILD_SHARED_LIBS)
    add_library(JosephsOddBuilder_Sound SHARED ${JOBSOUND_COMPILE_THIS})
else()
    add_library(JosephsOddBuilder_Sound STATIC ${JOBSOUND_COMPILE_THIS})
    target_compile_definitions(JosephsOddBuilder_Sound PUBLIC JOBSOUND_STATIC_DEFINE)
endif()



set(JOBSOUND_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/jobsound_export.h")
generate_export_header(JosephsOddBuilder_Sound
    BASE_NAME JOBSOUND
    EXPORT_FILE_NAME "${JOBSOUND_EXPORT_HEADER}"
)

set_target_properties(JosephsOddBuilder_Sound PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    OUTPUT_NAME "jobsound"
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

target_include_directories(JosephsOddBuilder_Sound PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/backends/alsa>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/backends/alsa>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/backends/pipewire>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/backends/pipewire>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/dsp>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/dsp>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/midi>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/midi>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/codecs>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/codecs/pcm>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs/pcm>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/codecs/opus>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs/opus>

    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/codecs/flac>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs/flac>

    ## export file
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>


    ${LibAlsa_INCLUDE_DIRS}
    ${LibPipewire_INCLUDE_DIRS}
    ${LibSPA_INCLUDE_DIRS}
    ${LibWirePlumber_INCLUDE_DIRS}
)


target_link_libraries(JosephsOddBuilder_Sound PRIVATE
    JosephsOddBuilder_Core
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Threads
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

if(JOB_LINUX)
    target_link_libraries(JosephsOddBuilder_Sound PUBLIC atomic)
elseif(JOB_WINDOWS)
    target_compile_options(JosephsOddBuilder_Sound PRIVATE /wd4251)
endif()


install(TARGETS JosephsOddBuilder_Sound
    EXPORT JosephsOddBuilderSoundTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound
)

install(FILES ${JOBSOUND_EXPORT_HEADER}             DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound)
install(FILES ${JOBSOUND_BACKEND_ALSA_HEADERS}      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/backend/alsa)
install(FILES ${JOBSOUND_BACKEND_PIPEWIRE_HEADERS}  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/backend/pipewire)
install(FILES ${JOBSOUND_DSP_HEADERS}               DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/dsp)
install(FILES ${JOBSOUND_MIDI_HEADERS}              DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/midi)
install(FILES ${JOBSOUND_CODECS_HEADERS}            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs)
install(FILES ${JOBSOUND_CODECS_PCM_HEADERS}        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs/pcm)
install(FILES ${JOBSOUND_CODECS_OPUS_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs/opus)
install(FILES ${JOBSOUND_CODECS_FLAC_HEADERS}       DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jobsound/codecs/flac)

if(JOB_LINUX)
    set(PKGCONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(PKGCONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/jobsound.pc")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/jobsound.pc.in"
        "${PKGCONFIG_FILE}"
        @ONLY
    )

    install(FILES "${PKGCONFIG_FILE}" DESTINATION "${PKGCONFIG_INSTALL_DIR}" )
endif()

install(EXPORT JosephsOddBuilderSoundTargets
    FILE JosephsOddBuilderSoundTargets.cmake
    NAMESPACE JOB::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/JosephsOddBuilderSound"
)

