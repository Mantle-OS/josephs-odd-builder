#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "model_config.h"
#include "jobmodel_export.h"

namespace job::model {

// Common contract for anything that can populate a ModelConfig from an
// on-disk model description. Not a pure interface derived readers
// share a validate-and-log tail via finalizeAndValidate() instead of
// each reinventing it.
class JOBMODEL_EXPORT IModelConfigReader
{
public:
    using Ptr  = std::shared_ptr<IModelConfigReader>;
    using WPtr = std::weak_ptr<IModelConfigReader>;
    using UPtr = std::unique_ptr<IModelConfigReader>;

    IModelConfigReader() = default;
    virtual ~IModelConfigReader() = default;

    IModelConfigReader(const IModelConfigReader &) = default;
    IModelConfigReader &operator=(const IModelConfigReader &) = default;
    IModelConfigReader(IModelConfigReader &&) noexcept = default;
    IModelConfigReader &operator=(IModelConfigReader &&) noexcept = default;

    // modelPath is whatever the format calls a "model root": a single
    // GGUF file, a HuggingFace directory containing config.json, a
    // SafeTensors index the derived reader decides what it expects.
    [[nodiscard]] virtual bool read(const std::filesystem::path &modelPath, ModelConfig &config) = 0;

protected:
    // Shared tail: every format ends with "did this produce a sane
    // config", so log + validate lives here once instead of per-reader.
    [[nodiscard]] static bool finalizeAndValidate(const ModelConfig &config, std::string_view readerTag);
};

} // namespace job::model
