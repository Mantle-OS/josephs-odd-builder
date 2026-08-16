#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <job_obj_hash.h>

#include "biquad_eq_31band.h"
#include "equalizerbank.h"
#include "virtual_eq_band.h"

#include "jobsound_export.h"
namespace job::sound {

class JOBSOUND_EXPORT VirtualEqProcessor
{
public:
    using BandModel = core::JobObjHashFast<VirtualEqBand::UPtr>;

    static VirtualEqProcessor *instance()
    {
        static VirtualEqProcessor s_instance;
        return &s_instance;
    }

    VirtualEqProcessor(const VirtualEqProcessor &) = delete;
    VirtualEqProcessor(VirtualEqProcessor &&) = delete;

    VirtualEqProcessor &operator=(const VirtualEqProcessor &) = delete;
    VirtualEqProcessor &operator=(VirtualEqProcessor &&) = delete;

    ~VirtualEqProcessor() = default;

    [[nodiscard]] float sampleRate() const noexcept
    {
        return m_sampleRate;
    }

    void setSampleRate(float rate)
    {
        if (m_sampleRate == rate)
            return;

        m_sampleRate = rate;
        updateEq();
    }

    [[nodiscard]] float qFactor() const noexcept
    {
        return m_qFactor;
    }

    void setQFactor(float q)
    {
        if (m_qFactor == q)
            return;

        m_qFactor = q;
        updateEq();
    }

    [[nodiscard]] BandModel *bandModel() noexcept
    {
        return m_bandModel.get();
    }

    [[nodiscard]] const BandModel *bandModel() const noexcept
    {
        return m_bandModel.get();
    }

    void setBandGain(int band, float gainDb)
    {
        if (band < 0 || band >= static_cast<int>(EQ_BAND_FREQUENCIES.size()))
            return;

        m_eq.setGain(band, gainDb);

        if (auto *bandObj = m_bandModel->at(std::to_string(band)))
            bandObj->setGain(gainDb);
    }

    [[nodiscard]] float processSample(float input) noexcept
    {
        return m_eq.processSample(input);
    }

    [[nodiscard]] std::vector<float> processBuffer(
        const std::vector<float> &inputSamples
        )
    {
        std::vector<float> output;
        output.reserve(inputSamples.size());

        for (const float sample : inputSamples)
            output.push_back(m_eq.processSample(sample));

        return output;
    }

private:
    explicit VirtualEqProcessor() :
        m_eq{m_sampleRate, m_qFactor},
        m_bandModel{std::make_unique<BandModel>()}
    {
        m_bandModel->reserve(EQ_BAND_FREQUENCIES.size());

        for (std::size_t i = 0; i < EQ_BAND_FREQUENCIES.size(); ++i) {
            auto band = VirtualEqBand::createUnique();

            band->setUid(std::to_string(i));
            band->setBandIndex(static_cast<int>(i));
            band->setFrequency(EQ_BAND_FREQUENCIES[i]);
            band->setGain(0.0f);

            m_bandModel->insert(std::move(band));
        }
    }

    void updateEq()
    {
        m_eq = EqualizerBank{
            m_sampleRate,
            m_qFactor
        };

        for (const auto &item : *m_bandModel) {
            const auto &band = item.second;
            if (!band)
                continue;

            m_eq.setGain(static_cast<std::size_t>(band->bandIndex()), band->gain());
        }
    }

private:
    float                       m_sampleRate = 48000.0f;
    float                       m_qFactor    = 1.0f;
    EqualizerBank               m_eq;
    std::unique_ptr<BandModel>  m_bandModel;
};

} // namespace job::sound