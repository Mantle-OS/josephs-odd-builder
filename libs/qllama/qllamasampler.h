#ifndef QLLAMASAMPLER_H
#define QLLAMASAMPLER_H

#include <QObject>
#include <QQmlEngine>
#include <llama.h>

#include <property-macros.h>
#include "qllamabase.h"


class QLlamaSampler : public QLlamaBase
{
    Q_OBJECT
    QML_ELEMENT

    QP_RW(qint32, topK,           40)
    QP_RW(double, topP,           0.95)
    QP_RW(double, minP,           0.05)
    QP_RW(double, temperature,    0.80)
    QP_RW(quint32, seed,          42)

    // XTC Truncation Boundaries
    QP_RW(double, xtcProbability, 0.00) // 0 = disabled
    QP_RW(double, xtcThreshold,   0.10)

    QP_RW(qint32, mirostatMode,   0)    // 0 = disabled, 1 = Mirostat v1, 2 = Mirostat v2
    QP_RW(double, mirostatTau,    5.0)  // target entropy
    QP_RW(double, mirostatEta,    0.1)  // learning rate

    // Fallback
    QP_RW(bool,   useGreedy,      false) // If true, forces absolute argmax step bypassing the chain

public:
    explicit QLlamaSampler(QObject *parent = nullptr) :
        QLlamaBase{parent},
        m_chain(nullptr)
    {
        auto markDirty = [this]() { m_isDirty = true; };

        connect(this, &QLlamaSampler::topKChanged,           this, markDirty);
        connect(this, &QLlamaSampler::topPChanged,           this, markDirty);
        connect(this, &QLlamaSampler::minPChanged,           this, markDirty);
        connect(this, &QLlamaSampler::temperatureChanged,    this, markDirty);
        connect(this, &QLlamaSampler::seedChanged,           this, markDirty);
        connect(this, &QLlamaSampler::xtcProbabilityChanged, this, markDirty);
        connect(this, &QLlamaSampler::xtcThresholdChanged,   this, markDirty);
        connect(this, &QLlamaSampler::mirostatModeChanged,   this, markDirty);
        connect(this, &QLlamaSampler::mirostatTauChanged,    this, markDirty);
        connect(this, &QLlamaSampler::mirostatEtaChanged,    this, markDirty);
        connect(this, &QLlamaSampler::useGreedyChanged,      this, markDirty);
    }

    ~QLlamaSampler() override
    {
        destroyChain();
    }

    void setVocabContextSize(int32_t size) {
        if (m_currentVocabSize != size) {
            m_currentVocabSize = size;
            m_isDirty = true;
        }
    }

    // Compiles or updates the underlying sampling architecture pipeline on demand ...I think
    struct llama_sampler* prepareSamplerChain() {
        if (!m_isDirty && m_chain)
            return m_chain;

        destroyChain();
        if (m_useGreedy) {
            m_chain = llama_sampler_init_greedy();
            m_isDirty = false;
            return m_chain;
        }

        auto sparams = llama_sampler_chain_default_params();
        m_chain = llama_sampler_chain_init(sparams);

        if (!m_chain)
            return nullptr;

        if (m_temperature > 0.0)
            llama_sampler_chain_add(m_chain, llama_sampler_init_temp(static_cast<float>(m_temperature)));
        if (m_topK > 0)
            llama_sampler_chain_add(m_chain, llama_sampler_init_top_k(m_topK));
        if (m_topP > 0.0 && m_topP < 1.0)
            llama_sampler_chain_add(m_chain, llama_sampler_init_top_p(static_cast<float>(m_topP), 1));
        if (m_minP > 0.0)
            llama_sampler_chain_add(m_chain, llama_sampler_init_min_p(static_cast<float>(m_minP), 1));

        if (m_xtcProbability > 0.0) {
            llama_sampler_chain_add(m_chain, llama_sampler_init_xtc(
                                                 static_cast<float>(m_xtcProbability),
                                                 static_cast<float>(m_xtcThreshold),
                                                 1,
                                                 m_seed
                                                 ));
        }

        if (m_mirostatMode == 1) {
            // Pull the actual active token pool count from your backend or default to a safe standardds
            int32_t vocabSize = m_currentVocabSize > 0 ? m_currentVocabSize : 32000;

            llama_sampler_chain_add(m_chain, llama_sampler_init_mirostat(
                                                 vocabSize,
                                                 m_seed,
                                                 static_cast<float>(m_mirostatTau),
                                                 static_cast<float>(m_mirostatEta),
                                                 100        // FIXME later 100 is okay for now
                                                 ));
        } else if (m_mirostatMode == 2) {
            llama_sampler_chain_add(m_chain, llama_sampler_init_mirostat_v2(
                                                 m_seed,
                                                 static_cast<float>(m_mirostatTau),
                                                 static_cast<float>(m_mirostatEta)
                                                 ));
        }
        else {
            // Default distribution selection pass based on accumulated weights
            llama_sampler_chain_add(m_chain, llama_sampler_init_dist(m_seed));
        }

        m_isDirty = false;
        return m_chain;
    }

    void destroyChain() {
        if (m_chain) {
            llama_sampler_free(m_chain);
            m_chain = nullptr;
        }
    }

private:
    struct llama_sampler *m_chain = nullptr;
    int32_t m_currentVocabSize = 0;
    bool m_isDirty = true;
};


#endif // QLLAMASAMPLER_H