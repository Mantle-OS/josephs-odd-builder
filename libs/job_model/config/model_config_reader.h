#pragma once

#include <filesystem>
#include <string>

#include "model_config.h"
#include "job_gguf.h"
#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT ModelConfigReader {
public:
    // Read model configuration from an already-opened JobGguf container
    [[nodiscard]] static bool readFromGguf(const ggml::JobGguf& gguf, ModelConfig& config);

    // Convenience method to open a GGUF file and read its configuration
    [[nodiscard]] static bool readFromFile(const std::filesystem::path& ggufPath, ModelConfig& config);

    // Read model config from a HuggingFace directory containing config.json & generation_config.json
    [[nodiscard]] static bool readFromJsonDirectory(const std::filesystem::path& dirPath, ModelConfig& config);
};

} // namespace job::model