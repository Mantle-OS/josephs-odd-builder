#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "model_config.h"
#include "jobmodel_export.h"

namespace job::model  {

template <typename T>
[[nodiscard]] T jsonValueOr(const nlohmann::json &j, const std::string &key, T defaultValue)
{
    if (j.contains(key) && !j[key].is_null()) {
        try {
            return j[key].get<T>();
        } catch (...) {
            return defaultValue;
        }
    }

    return defaultValue;
}

JOBMODEL_EXPORT void populateModelConfigFromHfJson(const nlohmann::json &configJson,
                                                   const std::string &modelDirName,
                                                   ModelConfig &config);

JOBMODEL_EXPORT void populateSamplerFromHfGenerationConfig(const nlohmann::json &generationConfigJson,
                                                           ModelConfig &config);

} // namespace job::model