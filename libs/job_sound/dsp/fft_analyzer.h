#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <functional>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

#include "jobsound_export.h"
namespace job::sound {

class JOBSOUND_EXPORT FftAnalyzer
{
public:
    using Ptr  = std::shared_ptr<FftAnalyzer>;
    using WPtr = std::weak_ptr<FftAnalyzer>;
    using UPtr = std::unique_ptr<FftAnalyzer>;

    using WindowChanged = std::function<void()>;

    enum WindowType {
        Hann = 0,
        BlackmanHarris = 1
    };

    static Ptr createShared(int fftSize, WindowType type = Hann)
    {
        return std::make_shared<FftAnalyzer>(fftSize, type);
    }

    static UPtr createUnique(int fftSize, WindowType type = Hann)
    {
        return std::make_unique<FftAnalyzer>(fftSize, type);
    }

    explicit FftAnalyzer(int fftSize, WindowType type = Hann)
        : m_fftSize(fftSize),
        m_sampleRate(48000.0f),
        m_windowType(type)
    {
        m_input.reserve(fftSize);
        m_window.resize(fftSize);
        m_peakHold.assign(fftSize / 2, -90.0f);

        updateWindow();
    }

    FftAnalyzer(const FftAnalyzer &) = default;
    FftAnalyzer(FftAnalyzer &&) noexcept = default;

    FftAnalyzer &operator=(const FftAnalyzer &) = default;
    FftAnalyzer &operator=(FftAnalyzer &&) noexcept = default;

    ~FftAnalyzer() = default;

    void setSampleRate(float rate) noexcept
    {
        m_sampleRate = rate;
    }

    [[nodiscard]] float sampleRate() const noexcept
    {
        return m_sampleRate;
    }

    [[nodiscard]] int fftSize() const noexcept
    {
        return m_fftSize;
    }

    [[nodiscard]] int windowType() const noexcept
    {
        return static_cast<int>(m_windowType);
    }

    void setWindowType(int type)
    {
        const WindowType newType = static_cast<WindowType>(type);

        if (newType == m_windowType)
            return;

        m_windowType = newType;
        updateWindow();

        if (m_windowChanged)
            m_windowChanged();
    }

    void setWindowChanged(WindowChanged callback)
    {
        m_windowChanged = std::move(callback);
    }

    void pushSamples(const float *samples, int count)
    {
        m_input.clear();

        for (int i = 0; i < std::min(count, m_fftSize); ++i)
            m_input.push_back(samples[i]);
    }

    [[nodiscard]] std::vector<float> compute(const float *samples)
    {
        std::vector<std::complex<float>> input(m_fftSize);

        for (int i = 0; i < m_fftSize; ++i)
            input[i] = std::complex<float>(samples[i] * m_window[i], 0.0f);

        fft(input);

        std::vector<float> magnitude;
        magnitude.reserve(m_fftSize / 2);

        float maxMag = 1e-6f;

        for (int i = 0; i < m_fftSize / 2; ++i)
            maxMag = std::max(maxMag, std::abs(input[i]));

        for (int i = 0; i < m_fftSize / 2; ++i) {
            const float db = 20.0f * std::log10(std::abs(input[i]) / maxMag + 1e-6f);
            magnitude.push_back(db);
            float &peak = m_peakHold[i];
            if (db > peak)
                peak = db;
            else
                peak -= 0.5f;
        }

        return magnitude;
    }

    [[nodiscard]] const std::vector<float> &peakHold() const noexcept
    {
        return m_peakHold;
    }

private:
    void updateWindow()
    {
        const float pi = std::numbers::pi_v<float>;

        for (int i = 0; i < m_fftSize; ++i) {
            if (m_windowType == Hann) {
                m_window[i] = 0.5f * (1.0f - std::cos(2.0f * pi * static_cast<float>(i) / static_cast<float>(m_fftSize - 1)));
            } else if (m_windowType == BlackmanHarris) {
                const float x =
                    pi * static_cast<float>(i) /
                    static_cast<float>(m_fftSize - 1);

                m_window[i] =
                    0.35875f
                    - 0.48829f * std::cos(2.0f * x)
                    + 0.14128f * std::cos(4.0f * x)
                    - 0.01168f * std::cos(6.0f * x);
            }
        }
    }

    void fft(std::vector<std::complex<float>> &a)
    {
        const std::size_t N = a.size();

        if (N <= 1)
            return;

        std::vector<std::complex<float>> even(N / 2);
        std::vector<std::complex<float>> odd(N / 2);

        for (std::size_t i = 0; i < N / 2; ++i) {
            even[i] = a[i * 2];
            odd[i]  = a[i * 2 + 1];
        }

        fft(even);
        fft(odd);
        const float pi = std::numbers::pi_v<float>;
        for (std::size_t k = 0; k < N / 2; ++k) {
            const std::complex<float> t =
                std::polar(
                    1.0f,
                    -2.0f * pi * static_cast<float>(k) / static_cast<float>(N)
                    ) * odd[k];

            a[k]         = even[k] + t;
            a[k + N / 2] = even[k] - t;
        }
    }

private:
    int                 m_fftSize;
    float               m_sampleRate;
    WindowType          m_windowType;
    WindowChanged       m_windowChanged;

    std::vector<float>  m_input;
    std::vector<float>  m_window;
    std::vector<float>  m_peakHold;
};

} // namespace job::sound
