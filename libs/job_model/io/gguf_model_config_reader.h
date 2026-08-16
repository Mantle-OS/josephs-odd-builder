#pragma once

#include <memory>

#include <job_gguf.h>

#include "imodel_config_reader.h"
#include "jobmodel_export.h"

namespace job::model {

// Populates a ModelConfig from GGUF metadata key/value pairs.
class JOBMODEL_EXPORT GgufModelConfigReader final : public IModelConfigReader
{
public:
    using Ptr  = std::shared_ptr<GgufModelConfigReader>;
    using WPtr = std::weak_ptr<GgufModelConfigReader>;
    using UPtr = std::unique_ptr<GgufModelConfigReader>;

    GgufModelConfigReader() = default;
    ~GgufModelConfigReader() override = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<GgufModelConfigReader>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<GgufModelConfigReader>(); }

    GgufModelConfigReader(const GgufModelConfigReader &) = default;
    GgufModelConfigReader &operator=(const GgufModelConfigReader &) = default;
    GgufModelConfigReader(GgufModelConfigReader &&) noexcept = default;
    GgufModelConfigReader &operator=(GgufModelConfigReader &&) noexcept = default;

    // modelPath is a single .gguf file. Opens it, delegates to readFromGguf().
    [[nodiscard]] bool read(const std::filesystem::path &modelPath, ModelConfig &config) override;

    // Extra helper beyond the interface contract: read directly from an
    // already-open JobGguf container, for callers (JobModel::load) that
    // already hold one and shouldn't open the file twice.
    [[nodiscard]] bool readFromGguf(const ggml::JobGguf &gguf, ModelConfig &config);
};

} // namespace job::model