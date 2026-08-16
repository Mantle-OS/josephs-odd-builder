#pragma once

#include "biquad_eq_31band.h"
#include "jobsound_export.h"

namespace job::sound {
class JOBSOUND_EXPORT EqualizerBank {
public:
    EqualizerBank(float sampleRate = 48000.0f, float q = 1.0f)
        : m_sampleRate(sampleRate), m_q(q)
    {
        for (size_t i = 0; i < EQ_BAND_FREQUENCIES.size(); ++i) {
            m_filters[i].configure(BiquadFilter::Peaking, EQ_BAND_FREQUENCIES[i], 0.0f, q, sampleRate);
        }
    }

    void setGain(int band, float gainDb) {
        if (band >= 0 && band < (int)m_filters.size()) {
            m_filters[band].configure(BiquadFilter::Peaking, EQ_BAND_FREQUENCIES[band], gainDb, m_q, m_sampleRate);
        }
    }

    float processSample(float sample) {
        for (auto &filter : m_filters)
            sample = filter.process(sample);
        return sample;
    }

    void processBuffer(const float *input, float *output, int frameCount) {
        for (int i = 0; i < frameCount; ++i) {
            float sample = input[i];
            for (auto &filter : m_filters)
                sample = filter.process(sample);
            output[i] = sample;
        }
    }

private:
    float m_sampleRate;
    float m_q;
    std::array<BiquadFilter, 31> m_filters;
};
}