#pragma once

#include <memory>

#include "imodel_config_reader.h"
#include "jobmodel_export.h"

namespace job::model {

// SafeTensors models ship the same config.json/generation_config.json
// pair as HfJsonModelConfigReader targets -- weight tensor shapes alone
// can't recover most hyperparameters (rope_theta, rms_norm_eps, the
// architecture name itself), so config.json remains the primary source.
// This reader adds two things HfJsonModelConfigReader doesn't need:
// confirming actual safetensors weight files exist (single model.safetensors
// or a sharded model.safetensors.index.json), and falling back to a
// tensor-name-derived blockCount when config.json is missing
// "num_hidden_layers" -- see model_config_reader_utils.h.
class JOBMODEL_EXPORT SafeTensorsModelConfigReader final : public IModelConfigReader
{
public:
    using Ptr  = std::shared_ptr<SafeTensorsModelConfigReader>;
    using WPtr = std::weak_ptr<SafeTensorsModelConfigReader>;
    using UPtr = std::unique_ptr<SafeTensorsModelConfigReader>;

    SafeTensorsModelConfigReader() = default;
    ~SafeTensorsModelConfigReader() override = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<SafeTensorsModelConfigReader>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<SafeTensorsModelConfigReader>(); }

    SafeTensorsModelConfigReader(const SafeTensorsModelConfigReader &) = default;
    SafeTensorsModelConfigReader &operator=(const SafeTensorsModelConfigReader &) = default;
    SafeTensorsModelConfigReader(SafeTensorsModelConfigReader &&) noexcept = default;
    SafeTensorsModelConfigReader &operator=(SafeTensorsModelConfigReader &&) noexcept = default;

    // modelPath is a directory containing config.json plus either
    // model.safetensors or model.safetensors.index.json.
    [[nodiscard]] bool read(const std::filesystem::path &modelPath, ModelConfig &config) override;
};

} // namespace job::model