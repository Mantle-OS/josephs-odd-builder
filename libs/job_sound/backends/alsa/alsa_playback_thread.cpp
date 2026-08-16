#include "alsa_playback_thread.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <alsa/asoundlib.h>

#include <job_logger.h>

namespace job::sound {

AlsaPlaybackThread::AlsaPlaybackThread(const std::string &device):
    m_deviceName(device)
{
    m_thread.setRunFunction([this](std::stop_token token) {
        run(token);
    });
}

AlsaPlaybackThread::~AlsaPlaybackThread()
{
    stop();
    (void)m_thread.join();
}

bool AlsaPlaybackThread::isRunning() const noexcept
{
    return m_thread.isRunning();
}

threads::JobThread::StartResult AlsaPlaybackThread::start()
{
    return m_thread.start();
}

void AlsaPlaybackThread::stop() noexcept
{
    m_thread.requestStop();
}

bool AlsaPlaybackThread::join() noexcept
{
    return m_thread.join();
}

const std::string &AlsaPlaybackThread::deviceName() const noexcept
{
    return m_deviceName;
}

void AlsaPlaybackThread::setDeviceName(const std::string &device)
{
    if (!device.empty() && device != m_deviceName)
        m_deviceName = device;
}

void AlsaPlaybackThread::setPlayedSamples(PlayedSamples callback)
{
    m_playedSamples = std::move(callback);
}

void AlsaPlaybackThread::enqueue(const std::vector<float> &samples)
{
    if (samples.empty())
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.insert(m_queue.end(), samples.begin(), samples.end());
}

void AlsaPlaybackThread::run(std::stop_token token) {
    snd_pcm_t* rawHandle = nullptr;
    if (const int err = snd_pcm_open(&rawHandle, m_deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0); err < 0) {
        JOB_LOG_WARN("Cannot open playback device '{}': {}", m_deviceName, snd_strerror(err));
        return;
    }
    std::unique_ptr<snd_pcm_t, decltype(&snd_pcm_close)> handle(rawHandle, &snd_pcm_close);

    snd_pcm_hw_params_t* params = nullptr;
    snd_pcm_hw_params_alloca(&params);

    if (snd_pcm_hw_params_any(handle.get(), params) < 0 ||
        snd_pcm_hw_params_set_access(handle.get(), params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0 ||
        snd_pcm_hw_params_set_format(handle.get(), params, SND_PCM_FORMAT_S16_LE) < 0) {
        JOB_LOG_WARN("Failed configuring base PCM parameters for '{}'", m_deviceName);
        return;
    }

    // Negotiate channels: prefer 1 (mono), fall back to nearest supported (e.g. 2 for stereo)
    unsigned int channels = 1;
    if (snd_pcm_hw_params_set_channels_near(handle.get(), params, &channels) < 0) {
        JOB_LOG_WARN("Cannot negotiate ALSA channel count: {}", m_deviceName);
        return;
    }

    unsigned int rate = 48000;
    snd_pcm_uframes_t bufferSize = 1024;
    if (snd_pcm_hw_params_set_rate_near(handle.get(), params, &rate, nullptr) < 0 ||
        snd_pcm_hw_params_set_buffer_size_near(handle.get(), params, &bufferSize) < 0 ||
        snd_pcm_hw_params(handle.get(), params) < 0) {
        JOB_LOG_WARN("Failed finalizing hardware parameters on '{}'", m_deviceName);
        return;
    }

    constexpr std::size_t kFramesPerChunk = 512;
    std::vector<float> monoChunk;
    std::vector<std::int16_t> interleavedPcm;

    monoChunk.reserve(kFramesPerChunk);
    interleavedPcm.reserve(kFramesPerChunk * channels);

    while (!token.stop_requested()) {
        monoChunk.clear();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            const std::size_t count = std::min(kFramesPerChunk, m_queue.size());
            if (count > 0) {
                monoChunk.assign(m_queue.begin(), m_queue.begin() + static_cast<std::ptrdiff_t>(count));
                m_queue.erase(m_queue.begin(), m_queue.begin() + static_cast<std::ptrdiff_t>(count));
            }
        }

        if (monoChunk.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const std::size_t frames = monoChunk.size();
        interleavedPcm.resize(frames * channels);

        // Convert float [-1.0, 1.0] -> S16_LE and replicate mono signal across all channels
        for (std::size_t f = 0; f < frames; ++f) {
            const float clamped = std::clamp(monoChunk[f], -1.0f, 1.0f);
            const auto s16 = static_cast<std::int16_t>(clamped * 32767.0f);
            for (unsigned int ch = 0; ch < channels; ++ch)
                interleavedPcm[f * channels + ch] = s16;
        }

        const snd_pcm_sframes_t rc = snd_pcm_writei(handle.get(), interleavedPcm.data(), frames);
        if (rc == -EPIPE || rc == -ESTRPIPE) {
            snd_pcm_recover(handle.get(), static_cast<int>(rc), 1);
            continue;
        }

        if (rc < 0) {
            JOB_LOG_WARN("ALSA write failed: {}", snd_strerror(static_cast<int>(rc)));
            snd_pcm_prepare(handle.get());
            continue;
        }

        if (m_playedSamples) {
            m_playedSamples(monoChunk);
        }
    }
}

}
