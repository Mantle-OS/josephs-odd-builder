#pragma once

#include <memory>

#include <job_base_obj.h>

#include "jobmodel_export.h"

namespace job::model {

// Normalization epsilon values.
// Pulled out of TransformerConfig into its own class -- matching how
// you already called out "RMS Norm" as its own graph-split piece,
// separate from attention/FFN/etc -- even though (unlike attention
// geometry) these values are read at every norm site in the model
// (pre-attention, pre-FFN, final output), not exclusive to one block.
class JOBMODEL_EXPORT NormConfig : public job::core::BaseObject
{
public:
    using Ptr  = std::shared_ptr<NormConfig>;
    using WPtr = std::weak_ptr<NormConfig>;
    using UPtr = std::unique_ptr<NormConfig>;

    NormConfig();
    ~NormConfig() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<NormConfig>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<NormConfig>(); }

    NormConfig(const NormConfig &) = default;
    NormConfig &operator=(const NormConfig &) = default;
    NormConfig(NormConfig &&) noexcept = default;
    NormConfig &operator=(NormConfig &&) noexcept = default;

    // Epsilon used by RMSNorm sites. Throws if value is non-finite or <= 0.
    [[nodiscard]] float rmsNormEps() const noexcept { return m_rmsNormEps; }
    void setRmsNormEps(float value);

    // Epsilon used by LayerNorm sites (non-RMS architectures). Throws if
    // value is non-finite or <= 0.
    [[nodiscard]] float layerNormEps() const noexcept { return m_layerNormEps; }
    void setLayerNormEps(float value);

    [[nodiscard]] bool isValid() const noexcept;

private:
    float m_rmsNormEps{1e-5f};
    float m_layerNormEps{1e-5f};
};

} // namespace job::model