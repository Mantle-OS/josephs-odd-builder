#pragma once

#include <cmath>
#include <array>
#include <vector>


#include "jobsound_export.h"

namespace job::sound {

// 31-band 1/3-octave ISO standard center frequencies
static constexpr std::array<float, 31> EQ_BAND_FREQUENCIES = {
    20.0f, 25.0f, 31.5f, 40.0f, 50.0f, 63.0f, 80.0f, 100.0f, 125.0f, 160.0f,
    200.0f, 250.0f, 315.0f, 400.0f, 500.0f, 630.0f, 800.0f, 1000.0f,
    1250.0f, 1600.0f, 2000.0f, 2500.0f, 3150.0f, 4000.0f, 5000.0f,
    6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f
};


class JOBSOUND_EXPORT BiquadFilter {
public:
    enum Type { Peaking }; // Expand later

    BiquadFilter() = default;

    void configure(Type type, float centerFreq, float gainDb, float q, float sampleRate) {
        if (gainDb == 0.0f) {
            m_b0 = 1.0f;
            m_b1 = 0.0f;
            m_b2 = 0.0f;
            m_a1 = 0.0f;
            m_a2 = 0.0f;
            return;
        }

        float A  = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * M_PI * centerFreq / sampleRate;
        float alpha = std::sin(w0) / (2.0f * q);
        float cos_w0 = std::cos(w0);

        float b0 = 1 + alpha * A;
        float b1 = -2 * cos_w0;
        float b2 = 1 - alpha * A;
        float a0 = 1 + alpha / A;
        float a1 = -2 * cos_w0;
        float a2 = 1 - alpha / A;

        m_b0 = b0 / a0;
        m_b1 = b1 / a0;
        m_b2 = b2 / a0;
        m_a1 = a1 / a0;
        m_a2 = a2 / a0;
    }

    float process(float in) {
        float out = m_b0 * in + m_b1 * m_x1 + m_b2 * m_x2
                    - m_a1 * m_y1 - m_a2 * m_y2;
        m_x2 = m_x1;
        m_x1 = in;
        m_y2 = m_y1;
        m_y1 = out;
        return out;
    }

private:
    float m_b0 = 0, m_b1 = 0, m_b2 = 0;
    float m_a1 = 0, m_a2 = 0;
    float m_x1 = 0, m_x2 = 0;
    float m_y1 = 0, m_y2 = 0;
};

}