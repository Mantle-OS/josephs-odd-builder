#include "alsa_capture_thread.h"

#include <cstdint>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

#include <alsa/asoundlib.h>
#include <job_logger.h>

namespace job::sound {

AlsaCaptureThread::AlsaCaptureThread(const std::string& device)
    : m_deviceName(device) {
    m_thread.setRunFunction([this](std::stop_token token) { run(token); });
}

AlsaCaptureThread::~AlsaCaptureThread() {
    stop();
    (void)m_thread.join();
}

bool AlsaCaptureThread::isRunning() const noexcept {
    return m_thread.isRunning();
}

threads::JobThread::StartResult AlsaCaptureThread::start() {
    return m_thread.start();
}

void AlsaCaptureThread::stop() noexcept {
    m_thread.requestStop();
}

bool AlsaCaptureThread::join() noexcept {
    return m_thread.join();
}

const std::string& AlsaCaptureThread::deviceName() const noexcept {
    return m_deviceName;
}

void AlsaCaptureThread::setDeviceName(const std::string& device) {
    if (!device.empty() && device != m_deviceName) {
        m_deviceName = device;
    }
}

void AlsaCaptureThread::setSamplesReady(SamplesReady callback) {
    m_samplesReady = std::move(callback);
}

void AlsaCaptureThread::run(std::stop_token token) {
    snd_pcm_t* rawHandle = nullptr;
    if (const int err = snd_pcm_open(&rawHandle, m_deviceName.c_str(), SND_PCM_STREAM_CAPTURE, 0); err < 0) {
        JOB_LOG_WARN("Cannot open capture device '{}': {}", m_deviceName, snd_strerror(err));
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

    // Negotiate channels: prefer 1 (mono), accept nearest supported (e.g. 2 for stereo)
    unsigned int channels = 1;
    if (snd_pcm_hw_params_set_channels_near(handle.get(), params, &channels) < 0) {
        JOB_LOG_WARN("Cannot negotiate ALSA capture channel count for '{}'", m_deviceName);
        return;
    }

    unsigned int rate = 48000;
    snd_pcm_uframes_t bufferSize = 1024;
    if (snd_pcm_hw_params_set_rate_near(handle.get(), params, &rate, nullptr) < 0 ||
        snd_pcm_hw_params_set_buffer_size_near(handle.get(), params, &bufferSize) < 0 ||
        snd_pcm_hw_params(handle.get(), params) < 0) {
        JOB_LOG_WARN("Failed finalizing hardware parameters for '{}'", m_deviceName);
        return;
    }

    constexpr snd_pcm_uframes_t kFramesPerChunk = 512;
    std::vector<std::int16_t> interleavedPcm(kFramesPerChunk * channels);
    std::vector<float> monoBuffer;
    monoBuffer.reserve(kFramesPerChunk);

    const float channelScale = 1.0f / (32768.0f * static_cast<float>(channels));

    while (!token.stop_requested()) {
        // Read interleaved frames from hardware
        const snd_pcm_sframes_t framesRead = snd_pcm_readi(handle.get(), interleavedPcm.data(), kFramesPerChunk);

        if (framesRead == -EPIPE || framesRead == -ESTRPIPE) {
            snd_pcm_recover(handle.get(), static_cast<int>(framesRead), 1);
            continue;
        }

        if (framesRead < 0) {
            JOB_LOG_WARN("ALSA capture read error on '{}': {}", m_deviceName, snd_strerror(static_cast<int>(framesRead)));
            break;
        }

        if (framesRead == 0) {
            continue;
        }

        monoBuffer.resize(static_cast<std::size_t>(framesRead));

        // Downmix multi-channel / convert S16_LE -> normalized float [-1.0f, 1.0f]
        for (std::size_t f = 0; f < static_cast<std::size_t>(framesRead); ++f) {
            float sum = 0.0f;
            const std::size_t frameOffset = f * channels;

            for (unsigned int ch = 0; ch < channels; ++ch) {
                sum += static_cast<float>(interleavedPcm[frameOffset + ch]);
            }

            monoBuffer[f] = sum * channelScale;
        }

        if (m_samplesReady) {
            m_samplesReady(monoBuffer);
        }
    }
}

} // namespace job::sound