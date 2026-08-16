#pragma once

#include <pipewire/pipewire.h>
#include <pipewire/main-loop.h>
#include <pipewire/loop.h>
#include <spa/param/audio/format-utils.h>

#include <cmath>
#include <thread>
#include <atomic>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT  PipeWireToneSource {


public:
    explicit PipeWireToneSource()
    {
        pw_init(nullptr, nullptr);
    }

    ~PipeWireToneSource() {
        stop();
        pw_deinit();
    }

    void start() {
        if (m_running) return;

        m_loop = pw_main_loop_new(nullptr);
        m_stream = pw_stream_new_simple(
            pw_main_loop_get_loop(m_loop),
            "PipeWireToneSource",
            pw_properties_new(
                PW_KEY_MEDIA_TYPE, "Audio",
                PW_KEY_MEDIA_CATEGORY, "Playback",
                PW_KEY_MEDIA_ROLE, "Music",
                nullptr),
            &stream_events,
            this
            );

        struct spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
                .format = SPA_AUDIO_FORMAT_S16,
                .rate = m_sampleRate,
                .channels = 1
            );

        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
        params[0] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &info);

        pw_stream_connect(
            m_stream,
            PW_DIRECTION_OUTPUT,
            PW_ID_ANY,
            (pw_stream_flags)(
                PW_STREAM_FLAG_AUTOCONNECT |
                PW_STREAM_FLAG_MAP_BUFFERS |
                PW_STREAM_FLAG_RT_PROCESS),
            params, 1
            );

        m_thread = std::thread([this] {
            m_running = true;
            pw_main_loop_run(m_loop);
        });
    }

    void stop() {
        if (!m_running) return;
        m_running = false;

        if (m_stream && m_loop) {
            pw_loop_invoke(
                pw_main_loop_get_loop(m_loop),
                [](spa_loop *loop, bool block, uint32_t seq, const void *inputData,
                   size_t inputSize, void *user_data) -> int {
                    auto *self = static_cast<PipeWireToneSource *>(user_data);
                    if (self->m_stream) {
                        pw_stream_destroy(self->m_stream);
                        self->m_stream = nullptr;
                    }
                    return 0;
                },
                0, // seq
                nullptr, 0, // no input data
                false, // block
                this // user_data
                );
        }

        if (m_loop) {
            pw_main_loop_quit(m_loop);
            if (m_thread.joinable()) m_thread.join();
            pw_main_loop_destroy(m_loop);
            m_loop = nullptr;
        }
    }

private:
    double m_frequency = 440.0;
    double m_amplitude = 0.5  ;

    static void on_process(void *userdata) {
        auto *self = static_cast<PipeWireToneSource *>(userdata);
        struct pw_buffer *b = pw_stream_dequeue_buffer(self->m_stream);
        if (!b) return;

        struct spa_buffer *buf = b->buffer;
        int16_t *dst = static_cast<int16_t *>(buf->datas[0].data);
        int n_samples = buf->datas[0].maxsize / sizeof(int16_t);

        for (int i = 0; i < n_samples; ++i) {
            dst[i] = static_cast<int16_t>(self->m_amplitude * 32767.0 * std::sin(self->m_phase));
            self->m_phase += 2.0 * M_PI * self->m_frequency / self->m_sampleRate;
            if (self->m_phase >= 2.0 * M_PI) self->m_phase -= 2.0 * M_PI;
        }

        pw_stream_queue_buffer(self->m_stream, b);
    }

    static const struct pw_stream_events    stream_events;
    pw_main_loop                            *m_loop = nullptr;
    pw_stream                               *m_stream = nullptr;
    std::thread                             m_thread;
    std::atomic<bool>                       m_running = false;
    // Tone generation state
    double                                  m_phase = 0.0;
    unsigned int                            m_sampleRate = 44100;
};

}