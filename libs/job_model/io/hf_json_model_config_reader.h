#pragma once

#include <memory>

#include "imodel_config_reader.h"
#include "jobmodel_export.h"

namespace job::model {

// Populates a ModelConfig from a HuggingFace-style directory:
// config.json (required) and generation_config.json (optional, sampler-only).
class JOBMODEL_EXPORT HfJsonModelConfigReader final : public IModelConfigReader
{
public:
    using Ptr  = std::shared_ptr<HfJsonModelConfigReader>;
    using WPtr = std::weak_ptr<HfJsonModelConfigReader>;
    using UPtr = std::unique_ptr<HfJsonModelConfigReader>;

    HfJsonModelConfigReader() = default;
    ~HfJsonModelConfigReader() override = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<HfJsonModelConfigReader>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<HfJsonModelConfigReader>(); }

    HfJsonModelConfigReader(const HfJsonModelConfigReader &) = default;
    HfJsonModelConfigReader &operator=(const HfJsonModelConfigReader &) = default;
    HfJsonModelConfigReader(HfJsonModelConfigReader &&) noexcept = default;
    HfJsonModelConfigReader &operator=(HfJsonModelConfigReader &&) noexcept = default;

    // modelPath is a directory containing config.json.
    [[nodiscard]] bool read(const std::filesystem::path &modelPath, ModelConfig &config) override;
};

} // namespace job::model