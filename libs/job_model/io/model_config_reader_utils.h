#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <job_gguf.h>

#include "model_config.h"
#include "jobmodel_export.h"

namespace job::model::io_util {

// Reads `key` from a parsed JSON object, tolerating a missing key, an
// explicit JSON null, or a type mismatch -- all fall back to
// defaultValue rather than throwing. Shared by every reader that parses
// a JSON sidecar file: HfJsonModelConfigReader and
// SafeTensorsModelConfigReader both use it, directly and via
// populateModelConfigFromHfJson()/populateSamplerFromHfGenerationConfig() below.
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

// Reads an architecture-prefixed GGUF integer key (e.g. "llama.block_count"),
// falling back to the "general."-prefixed key, then to defaultValue.
// Tolerates a missing key or a type mismatch.
[[nodiscard]] JOBMODEL_EXPORT uint32_t ggufValueU32(const ggml::JobGguf &gguf,
                                                    std::string_view archName,
                                                    std::string_view keySuffix,
                                                    uint32_t defaultValue);

// Same as ggufValueU32, but for floating-point keys.
[[nodiscard]] JOBMODEL_EXPORT float ggufValueFloat(const ggml::JobGguf &gguf,
                                                   std::string_view archName,
                                                   std::string_view keySuffix,
                                                   float defaultValue);

// Populates config's sub-configs from an already-parsed HuggingFace-style
// config.json object. Shared by HfJsonModelConfigReader and
// SafeTensorsModelConfigReader -- both target the same directory shape,
// differing only in weight-file format (pytorch_model.bin/safetensors
// aren't this function's concern at all). modelDirName is used as the
// modelName fallback, same as both readers did individually before.
// Throws std::invalid_argument if a hyperparameter is out of range (the
// throwing float setters on AttentionConfig/NormConfig/RopeConfig/
// OutputHeadConfig) -- callers decide how to log/report that.
JOBMODEL_EXPORT void populateModelConfigFromHfJson(const nlohmann::json &configJson,
                                                   const std::string &modelDirName,
                                                   ModelConfig &config);

// Populates SamplerConfig from an already-parsed HuggingFace-style
// generation_config.json object. Shared the same way as
// populateModelConfigFromHfJson -- this file is optional and non-fatal
// to fail parsing in both readers, so this never throws on its own; it
// only reads through jsonValueOr.
JOBMODEL_EXPORT void populateSamplerFromHfGenerationConfig(const nlohmann::json &generationConfigJson,
                                                           ModelConfig &config);

// Scans safetensors tensor names for the highest "<prefix>.layers.N."
// (or ".h.N." GPT-2/Bloom style) index present, returning N + 1 as a
// block-count estimate. Returns 0 if no layer-indexed tensor name was
// found. This is a fallback, not a general-purpose tensor-name parser --
// it only recovers blockCount, since that's the one field genuinely
// readable from names alone with no shape-reading required. Used by
// SafeTensorsModelConfigReader when config.json is missing the
// "num_hidden_layers" key.
[[nodiscard]] JOBMODEL_EXPORT uint32_t estimateBlockCountFromTensorNames(const std::vector<std::string> &tensorNames);

} // namespace job::model::io_util