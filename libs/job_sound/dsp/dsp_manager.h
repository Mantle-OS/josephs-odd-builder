#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <job_async_event_loop.h>
#include <job_logger.h>

#include "alsa_capture_thread.h"
#include "alsa_playback_thread.h"
#include "fft_analyzer.h"
#include "virtual_eq_processor.h"
#include "virtual_eq_band.h"

#include "jobsound_export.h"

namespace job::sound {

class JOBSOUND_EXPORT AudioProcessingManager
{
public:
    using Ptr  = std::shared_ptr<AudioProcessingManager>;
    using WPtr = std::weak_ptr<AudioProcessingManager>;
    using UPtr = std::unique_ptr<AudioProcessingManager>;

    enum class StreamType : std::uint8_t {
        Capture,
        Playback
    };

    struct SpectrumPoint {
        float frequency = 0.0f;
        float magnitude = 0.0f;
    };

    using RenderRequested = std::function<void(const std::vector<float> &samples, StreamType type)>;
    using SpectrumReady = std::function<void(const std::vector<SpectrumPoint> &spectrum)>;
    using StateChanged = std::function<void()>;

    static Ptr createShared()
    {
        return std::make_shared<AudioProcessingManager>();
    }

    static UPtr createUnique()
    {
        return std::make_unique<AudioProcessingManager>();
    }

    explicit AudioProcessingManager()
        : m_analyzer(1024, FftAnalyzer::Hann)
    {
        m_analyzer.setSampleRate(m_sampleRate);
        auto &loop = job::threads::AsyncEventLoop::globalLoop();        
        m_timerId = loop.addTimer([this]() { update(); }, std::chrono::milliseconds(33), true);
    }

    AudioProcessingManager(const AudioProcessingManager &) = delete;
    AudioProcessingManager(AudioProcessingManager &&) = delete;

    AudioProcessingManager &operator=(const AudioProcessingManager &) = delete;
    AudioProcessingManager &operator=(AudioProcessingManager &&) = delete;

    ~AudioProcessingManager()
    {
        if (m_timerId != 0)
            job::threads::AsyncEventLoop::globalLoop().cancelTimer(m_timerId);

        stopCapture();
        stopPlayback();
    }

    // ----------------------------------------------------------------
    // State
    // ----------------------------------------------------------------
    [[nodiscard]] bool captureActive() const noexcept
    {
        return m_captureThread && m_captureThread->isRunning();
    }

    [[nodiscard]] bool playbackActive() const noexcept
    {
        return m_playbackThread && m_playbackThread->isRunning();
    }

    [[nodiscard]] bool eqBypass() const noexcept
    {
        return m_eqBypass;
    }

    void setEqBypass(bool bypass)
    {
        if (m_eqBypass == bypass)
            return;

        m_eqBypass = bypass;

        if (m_eqBypassChanged)
            m_eqBypassChanged();
    }

    [[nodiscard]] int fftSize() const noexcept
    {
        return m_analyzer.fftSize();
    }

    [[nodiscard]] float sampleRate() const noexcept
    {
        return m_sampleRate;
    }

    void setSampleRate(float rate)
    {
        if (m_sampleRate == rate)
            return;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_sampleRate = rate;
            m_analyzer.setSampleRate(rate);
        }

        if (m_sampleRateChanged)
            m_sampleRateChanged();
    }

    [[nodiscard]] int windowType() const noexcept
    {
        return m_analyzer.windowType();
    }

    void setWindowType(int type)
    {
        if (m_analyzer.windowType() == type)
            return;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_analyzer.setWindowType(type);
        }

        if (m_windowTypeChanged)
            m_windowTypeChanged();
    }

    // ----------------------------------------------------------------
    // Callbacks
    // ----------------------------------------------------------------
    void setRenderRequested(RenderRequested callback)
    {
        m_renderRequested = std::move(callback);
    }

    void setPlaybackSpectrumReady(SpectrumReady callback)
    {
        m_playbackSpectrumReady = std::move(callback);
    }

    void setPlaybackPeaksReady(SpectrumReady callback)
    {
        m_playbackPeaksReady = std::move(callback);
    }

    void setCaptureSpectrumReady(SpectrumReady callback)
    {
        m_captureSpectrumReady = std::move(callback);
    }

    void setCaptureActiveChanged(StateChanged callback)
    {
        m_captureActiveChanged = std::move(callback);
    }

    void setPlaybackActiveChanged(StateChanged callback)
    {
        m_playbackActiveChanged = std::move(callback);
    }

    void setEqBypassChanged(StateChanged callback)
    {
        m_eqBypassChanged = std::move(callback);
    }

    void setSampleRateChanged(StateChanged callback)
    {
        m_sampleRateChanged = std::move(callback);
    }

    void setWindowTypeChanged(StateChanged callback)
    {
        m_windowTypeChanged = std::move(callback);
    }

    // ----------------------------------------------------------------
    // EQ
    // ----------------------------------------------------------------
    void resetAllBands()
    {
        auto *processor = VirtualEqProcessor::instance();
        if (!processor)
            return;

        auto *model = processor->bandModel();
        if (!model)
            return;

        for (std::size_t i = 0; i < model->size(); ++i) {
            auto *band = model->at(static_cast<int>(i));
            if (!band)
                continue;

            band->setGain(0.0f);
            processor->setBandGain(static_cast<int>(i), 0.0f);
        }
    }

    // ----------------------------------------------------------------
    // Audio
    // ----------------------------------------------------------------
    void feedSamples(const std::vector<float> &samples)
    {
        if (samples.empty())
            return;

        std::vector<float> playbackBuffer;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastCapture = samples;
        }

        if (m_captureSpectrumReady || m_renderRequested) {
            if (m_renderRequested)
                m_renderRequested(samples, StreamType::Capture);
        }

        if (!m_playbackThread || !playbackActive())
            return;

        if (m_eqBypass)
            playbackBuffer = samples;
        else
            playbackBuffer = VirtualEqProcessor::instance()->processBuffer(samples);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastPlayback = playbackBuffer;
        }

        m_playbackThread->enqueue(playbackBuffer);

        if (m_renderRequested)
            m_renderRequested(playbackBuffer, StreamType::Playback);
    }

    // ----------------------------------------------------------------
    // Capture / playback lifecycle
    // ----------------------------------------------------------------
    void startCaptureFrom(const std::string &deviceName)
    {
        stopCapture();
        m_captureThread = AlsaCaptureThread::createUnique(deviceName);

        m_captureThread->setSamplesReady([this](const std::vector<float> &samples) {
            feedSamples(samples);
        });

        const auto result = m_captureThread->start();

        if (result != job::threads::JobThread::StartResult::Started) {
            JOB_LOG_WARN("Failed to start ALSA capture thread for device: {}", deviceName);
            m_captureThread.reset();
        }

        if (m_captureActiveChanged)
            m_captureActiveChanged();
    }

    void startPlaybackTo(const std::string &deviceName)
    {
        stopPlayback();
        m_playbackThread = AlsaPlaybackThread::createUnique(deviceName);
        const auto result = m_playbackThread->start();

        if (result != job::threads::JobThread::StartResult::Started) {
            JOB_LOG_WARN("Failed to start ALSA playback thread for device: {}", deviceName);
            m_playbackThread.reset();
        }

        if (m_playbackActiveChanged)
            m_playbackActiveChanged();
    }

    void stopCapture()
    {
        if (!m_captureThread)
            return;

        m_captureThread->stop();
        (void)m_captureThread->join();
        m_captureThread.reset();

        if (m_captureActiveChanged)
            m_captureActiveChanged();
    }

    void stopPlayback()
    {
        if (!m_playbackThread)
            return;

        m_playbackThread->stop();
        (void)m_playbackThread->join();
        m_playbackThread.reset();

        if (m_playbackActiveChanged)
            m_playbackActiveChanged();
    }

private:
    [[nodiscard]] std::vector<float> makeFftBuffer(const std::vector<float> &samples) const
    {
        std::vector<float> buffer(static_cast<std::size_t>(m_analyzer.fftSize()), 0.0f);
        const std::size_t count = std::min(buffer.size(), samples.size());
        std::copy_n(samples.begin(), count, buffer.begin());
        return buffer;
    }

    [[nodiscard]] std::vector<SpectrumPoint> makeSpectrum(const std::vector<float> &values) const
    {
        std::vector<SpectrumPoint> points;
        points.reserve(values.size());

        if (values.empty())
            return points;

        const float nyquist = m_sampleRate * 0.5f;
        const float count = static_cast<float>(values.size());

        for (std::size_t i = 0; i < values.size(); ++i) {
            points.push_back({
                static_cast<float>(i) * nyquist / count,
                values[i]
            });
        }

        return points;
    }

    void update()
    {
        std::vector<float> playback;
        std::vector<float> capture;

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            playback = m_lastPlayback;
            capture  = m_lastCapture;
        }

        if (m_playbackSpectrumReady && !playback.empty()) {
            const auto fftInput = makeFftBuffer(playback);

            std::vector<float> spectrum;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                spectrum = m_analyzer.compute(fftInput.data());
            }

            m_playbackSpectrumReady(makeSpectrum(spectrum));
        }

        if (m_playbackPeaksReady) {
            std::vector<float> peaks;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                peaks = m_analyzer.peakHold();
            }

            if (!peaks.empty())
                m_playbackPeaksReady(makeSpectrum(peaks));
        }

        if (m_captureSpectrumReady && !capture.empty()) {
            const auto fftInput = makeFftBuffer(capture);

            std::vector<float> spectrum;

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                spectrum = m_analyzer.compute(fftInput.data());
            }

            m_captureSpectrumReady(makeSpectrum(spectrum));
        }
    }

private:
    mutable std::mutex          m_mutex;
    float                       m_sampleRate = 48000.0f;
    bool                        m_eqBypass   = false;
    FftAnalyzer                 m_analyzer;
    std::vector<float>          m_lastPlayback;
    std::vector<float>          m_lastCapture;
    std::uint64_t               m_timerId = 0;
    AlsaCaptureThread::UPtr     m_captureThread;
    AlsaPlaybackThread::UPtr    m_playbackThread;
    RenderRequested             m_renderRequested;
    SpectrumReady               m_playbackSpectrumReady;
    SpectrumReady               m_playbackPeaksReady;
    SpectrumReady               m_captureSpectrumReady;
    StateChanged                m_captureActiveChanged;
    StateChanged                m_playbackActiveChanged;
    StateChanged                m_eqBypassChanged;
    StateChanged                m_sampleRateChanged;
    StateChanged                m_windowTypeChanged;
};

} // namespace job::sound