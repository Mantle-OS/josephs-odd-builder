#pragma once

#include <memory>

#include "jobmodel_export.h"

namespace job::model {

// Output/LM-head execution knobs.
// Pulled out of ArchConfig because both fields are decisions consumed at
// the same graph stage -- right after the last transformer block, at the
// final projection to logits -- not architecture identity. tieWordEmbeddings
// in particular already drives a concrete decision in ModelWeights
// (whether the output projection reuses the token-embedding tensor).
class JOBMODEL_EXPORT OutputHeadConfig
{
public:
    using Ptr  = std::shared_ptr<OutputHeadConfig>;
    using WPtr = std::weak_ptr<OutputHeadConfig>;
    using UPtr = std::unique_ptr<OutputHeadConfig>;

    OutputHeadConfig();
    ~OutputHeadConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<OutputHeadConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<OutputHeadConfig>(); }

    OutputHeadConfig(const OutputHeadConfig &) = default;
    OutputHeadConfig &operator=(const OutputHeadConfig &) = default;
    OutputHeadConfig(OutputHeadConfig &&) noexcept = default;
    OutputHeadConfig &operator=(OutputHeadConfig &&) noexcept = default;

    // Soft-capping applied to final LM-head logits before softmax (Gemma 2 style).
    // 0.0f means disabled. Throws if value is non-finite or negative.
    [[nodiscard]] float finalLogitSoftCapping() const noexcept { return m_finalLogitSoftCapping; }
    void setFinalLogitSoftCapping(float value);

    // Whether the output projection reuses the token-embedding weight matrix
    // instead of carrying its own. See ModelWeights::hasTiedEmbedding().
    [[nodiscard]] bool tieWordEmbeddings() const noexcept { return m_tieWordEmbeddings; }
    void setTieWordEmbeddings(bool value) noexcept { m_tieWordEmbeddings = value; }

    [[nodiscard]] bool isValid() const noexcept;

private:
    float m_finalLogitSoftCapping{0.0f};
    bool  m_tieWordEmbeddings{false};
};

} // namespace job::model