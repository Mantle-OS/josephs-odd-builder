#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <string>
#include <utility>
#include <vector>

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/utils/result.h>

#include <job_logger.h>
#include <job_thread.h>

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT PipeWireStream
{
public:
    using Ptr  = std::shared_ptr<PipeWireStream>;
    using WPtr = std::weak_ptr<PipeWireStream>;
    using UPtr = std::unique_ptr<PipeWireStream>;

    using SamplesReady =
        std::function<void(const std::vector<float> &samples)>;

    enum class Direction : std::uint8_t {
        Input,
        Output
    };

    static Ptr createShared(Direction direction)
    {
        return std::make_shared<PipeWireStream>(direction);
    }

    static UPtr createUnique(Direction direction)
    {
        return std::make_unique<PipeWireStream>(direction);
    }

    explicit PipeWireStream(Direction direction)
        : m_direction(direction)
    {
        ensurePipeWireInitialized();

        if (!initialize())
            return;

        m_thread.setRunFunction(
            [this](std::stop_token token) {
                run(token);
            }
            );

        const auto result = m_thread.start();

        if (result != job::threads::JobThread::StartResult::Started) {
            JOB_LOG_WARN("Failed to start PipeWire stream thread");
            cleanup();
        }
    }

    PipeWireStream(const PipeWireStream &) = delete;
    PipeWireStream(PipeWireStream &&) = delete;

    PipeWireStream &operator=(const PipeWireStream &) = delete;
    PipeWireStream &operator=(PipeWireStream &&) = delete;

    ~PipeWireStream()
    {
        stop();
        (void)m_thread.join();
        cleanup();
    }

    [[nodiscard]] Direction direction() const noexcept
    {
        return m_direction;
    }

    [[nodiscard]] bool isRunning() const noexcept
    {
        return m_thread.isRunning();
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return m_stream != nullptr && m_loop != nullptr;
    }

    void setSamplesReady(SamplesReady callback)
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        m_samplesReady = std::move(callback);
    }

    void enqueue(const std::vector<float> &samples)
    {
        if (samples.empty())
            return;

        std::lock_guard<std::mutex> lock(m_queueMutex);

        m_pendingSamples.insert(
            m_pendingSamples.end(),
            samples.begin(),
            samples.end()
            );
    }

    void stop() noexcept
    {
        m_thread.requestStop();

        if (m_loop)
            pw_main_loop_quit(m_loop);
    }

private:
    struct PipeWireRuntime
    {
        PipeWireRuntime()
        {
            pw_init(nullptr, nullptr);
        }

        ~PipeWireRuntime()
        {
            pw_deinit();
        }

        PipeWireRuntime(const PipeWireRuntime &) = delete;
        PipeWireRuntime &operator=(const PipeWireRuntime &) = delete;
    };

    static void ensurePipeWireInitialized()
    {
        static PipeWireRuntime runtime;
        (void)runtime;
    }

    [[nodiscard]] bool initialize()
    {
        m_loop = pw_main_loop_new(nullptr);

        if (!m_loop) {
            JOB_LOG_WARN("Failed to create PipeWire main loop");
            return false;
        }

        m_stream = pw_stream_new_simple(
            pw_main_loop_get_loop(m_loop),
            "JOB PipeWire Stream",
            pw_properties_new(
                PW_KEY_MEDIA_TYPE,
                "Audio",

                PW_KEY_MEDIA_CATEGORY,
                m_direction == Direction::Output
                    ? "Playback"
                    : "Capture",

                PW_KEY_MEDIA_ROLE,
                "Music",

                nullptr
                ),
            &s_streamEvents,
            this
            );

        if (!m_stream) {
            JOB_LOG_WARN("Failed to create PipeWire stream");
            cleanup();
            return false;
        }

        spa_audio_info_raw info = SPA_AUDIO_INFO_RAW_INIT(
                .format   = SPA_AUDIO_FORMAT_F32,
                .rate     = 48000,
                .channels = 1
            );

        const spa_pod *params[1];

        std::uint8_t buffer[1024];

        spa_pod_builder builder =
            SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        params[0] = spa_format_audio_raw_build(
            &builder,
            SPA_PARAM_EnumFormat,
            &info
            );

        const pw_direction direction =
            m_direction == Direction::Output
                ? PW_DIRECTION_OUTPUT
                : PW_DIRECTION_INPUT;

        /*
         * Do NOT use PW_STREAM_FLAG_RT_PROCESS here yet.
         *
         * onProcess() currently uses STL containers, mutexes and user
         * callbacks. Those are not appropriate operations for a PipeWire
         * real-time process callback.
         *
         * [[FIXME]] Move playback/capture transfer to an RT-safe ring
         * buffer before enabling PW_STREAM_FLAG_RT_PROCESS.
         */
        const auto flags = static_cast<pw_stream_flags>(
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS
            );

        const int result = pw_stream_connect(
            m_stream,
            direction,
            PW_ID_ANY,
            flags,
            params,
            1
            );

        if (result < 0) {
            JOB_LOG_WARN(
                "Failed to connect PipeWire stream: {}",
                spa_strerror(result)
                );

            cleanup();
            return false;
        }

        return true;
    }

    void run(std::stop_token token)
    {
        if (!m_loop)
            return;

        /*
         * pw_main_loop_run() blocks until pw_main_loop_quit().
         * stop() wakes it by calling pw_main_loop_quit().
         */
        const int result = pw_main_loop_run(m_loop);

        if (result < 0 && !token.stop_requested()) {
            JOB_LOG_WARN(
                "PipeWire main loop failed: {}",
                spa_strerror(result)
                );
        }
    }

    void cleanup() noexcept
    {
        if (m_stream) {
            pw_stream_destroy(m_stream);
            m_stream = nullptr;
        }

        if (m_loop) {
            pw_main_loop_destroy(m_loop);
            m_loop = nullptr;
        }
    }

    static void onProcess(void *data)
    {
        auto *self = static_cast<PipeWireStream *>(data);

        if (!self || !self->m_stream)
            return;

        pw_buffer *pwBuffer =
            pw_stream_dequeue_buffer(self->m_stream);

        if (!pwBuffer || !pwBuffer->buffer)
            return;

        spa_buffer *spaBuffer = pwBuffer->buffer;

        if (spaBuffer->n_datas == 0)
            return;

        spa_data &dataBuffer = spaBuffer->datas[0];

        if (!dataBuffer.data) {
            pw_stream_queue_buffer(self->m_stream, pwBuffer);
            return;
        }

        if (self->m_direction == Direction::Output)
            self->processOutput(pwBuffer, dataBuffer);
        else
            self->processInput(pwBuffer, dataBuffer);

        pw_stream_queue_buffer(self->m_stream, pwBuffer);
    }

    void processOutput(
        pw_buffer *pwBuffer,
        spa_data &dataBuffer)
    {
        auto *ptr = static_cast<float *>(dataBuffer.data);

        std::size_t frames =
            dataBuffer.maxsize / sizeof(float);

        if (pwBuffer->requested > 0) {
            frames = std::min(
                frames,
                static_cast<std::size_t>(pwBuffer->requested)
                );
        }

        std::size_t written = 0;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);

            while (
                written < frames &&
                !m_pendingSamples.empty()) {

                ptr[written] = m_pendingSamples.front();
                m_pendingSamples.pop_front();

                ++written;
            }
        }

        std::fill(
            ptr + written,
            ptr + frames,
            0.0f
            );

        if (dataBuffer.chunk) {
            dataBuffer.chunk->offset = 0;
            dataBuffer.chunk->stride = sizeof(float);
            dataBuffer.chunk->size =
                static_cast<std::uint32_t>(
                    frames * sizeof(float)
                    );
        }
    }

    void processInput(
        pw_buffer *,
        spa_data &dataBuffer)
    {
        if (!dataBuffer.chunk)
            return;

        const std::uint32_t offset =
            dataBuffer.chunk->offset;

        const std::uint32_t size =
            dataBuffer.chunk->size;

        if (offset >= dataBuffer.maxsize)
            return;

        const std::uint32_t available =
            dataBuffer.maxsize - offset;

        const std::uint32_t validSize =
            std::min(size, available);

        const auto *bytes =
            static_cast<const std::uint8_t *>(
                dataBuffer.data
                );

        const auto *samples =
            reinterpret_cast<const float *>(
                bytes + offset
                );

        const std::size_t frames =
            validSize / sizeof(float);

        if (frames == 0)
            return;

        std::vector<float> block(
            samples,
            samples + frames
            );

        SamplesReady callback;

        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            callback = m_samplesReady;
        }

        if (callback)
            callback(block);
    }

private:
    inline static const pw_stream_events s_streamEvents = {
        .version = PW_VERSION_STREAM_EVENTS,
        .process = &PipeWireStream::onProcess
    };

    Direction m_direction;

    pw_main_loop *m_loop = nullptr;
    pw_stream    *m_stream = nullptr;

    job::threads::JobThread m_thread;

    mutable std::mutex m_queueMutex;
    mutable std::mutex m_callbackMutex;

    std::deque<float> m_pendingSamples;

    SamplesReady m_samplesReady;
};

} // namespace job::sound