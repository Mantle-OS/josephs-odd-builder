#include "hf_json_model_config_reader.h"

#include <fstream>

#include <nlohmann/json.hpp>

#include <job_logger.h>

#include "model_config_reader_utils.h"

namespace job::model {

bool HfJsonModelConfigReader::read(const std::filesystem::path &modelPath, ModelConfig &config)
{
    config = ModelConfig{};

    auto configPath = modelPath / "config.json";
    if (!std::filesystem::exists(configPath)) {
        JOB_LOG_ERROR("[HfJsonModelConfigReader] config.json not found in directory: '{}'", modelPath.string());
        return false;
    }

    try {
        std::ifstream f(configPath);
        nlohmann::json j = nlohmann::json::parse(f);
        io_util::populateModelConfigFromHfJson(j, modelPath.filename().string(), config);
    } catch (const std::exception &e) {
        // Also catches std::invalid_argument from the throwing float
        // setters (rmsNormEps, ropeFreqBase, ...) on an out-of-range value.
        JOB_LOG_ERROR("[HfJsonModelConfigReader] Failed to parse config.json: {}", e.what());
        return false;
    }

    // Optional Generation Parameters (SamplerConfig)
    auto genConfigPath = modelPath / "generation_config.json";
    if (std::filesystem::exists(genConfigPath)) {
        try {
            std::ifstream gf(genConfigPath);
            nlohmann::json gj = nlohmann::json::parse(gf);
            io_util::populateSamplerFromHfGenerationConfig(gj, config);
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[HfJsonModelConfigReader] Failed to parse generation_config.json (non-fatal): {}", e.what());
        }
    }

    if (!finalizeAndValidate(config, "HfJsonModelConfigReader"))
        return false;


    // SHUT UP
    // JOB_LOG_INFO("[HfJsonModelConfigReader] Successfully parsed JSON config for '{}' (Arch: {}, Layers: {}, Ctx: {})",
    //              config.archConfig().modelName(), config.archConfig().archName(),
    //              config.transformerConfig().blockCount(),
    //              config.transformerConfig().contextLength());

    return true;
}

} // namespace job::model