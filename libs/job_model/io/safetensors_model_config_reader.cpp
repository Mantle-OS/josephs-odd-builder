#include "safetensors_model_config_reader.h"

#include <cstdint>
#include <fstream>
#include <vector>

#include <nlohmann/json.hpp>

#include <job_logger.h>

#include "model_config_reader_utils.h"

namespace job::model {

namespace {

// Reads the JSON header of a single .safetensors file: an 8-byte
// little-endian header length, followed by that many bytes of UTF-8 JSON
// mapping tensor name -> {dtype, shape, data_offsets}, plus an optional
// "__metadata__" string map. Returns the tensor names present (excluding
// "__metadata__"), or an empty vector on any read/parse failure.
std::vector<std::string> readSafetensorsTensorNames(const std::filesystem::path &path)
{
    std::vector<std::string> names;

    std::ifstream f(path, std::ios::binary);
    if (!f)
        return names;

    std::uint64_t headerLen = 0;
    f.read(reinterpret_cast<char *>(&headerLen), sizeof(headerLen));
    // Sanity cap -- a real header is kilobytes to low megabytes; a
    // gigabyte-plus "header length" means the file is corrupt or this
    // isn't actually a safetensors file.
    if (!f || headerLen == 0 || headerLen > (1ull << 30))
        return names;

    std::string headerBytes(headerLen, '\0');
    f.read(headerBytes.data(), static_cast<std::streamsize>(headerLen));
    if (!f)
        return names;

    try {
        nlohmann::json header = nlohmann::json::parse(headerBytes);
        for (auto it = header.begin(); it != header.end(); ++it) {
            if (it.key() != "__metadata__")
                names.push_back(it.key());
        }
    } catch (...) {
        // Malformed header -- treat as "no tensors found", caller decides.
    }

    return names;
}

// Reads tensor names from a sharded model.safetensors.index.json's
// "weight_map" keys. No shard files are opened -- names alone are enough
// for the blockCount fallback this feeds; shapes aren't needed for that.
std::vector<std::string> readSafetensorsIndexTensorNames(const std::filesystem::path &indexPath)
{
    std::vector<std::string> names;

    std::ifstream f(indexPath);
    if (!f)
        return names;

    try {
        nlohmann::json index = nlohmann::json::parse(f);
        if (index.contains("weight_map") && index["weight_map"].is_object()) {
            for (auto it = index["weight_map"].begin(); it != index["weight_map"].end(); ++it)
                names.push_back(it.key());
        }
    } catch (...) {
        // Malformed index -- treat as "no tensors found", caller decides.
    }

    return names;
}

} // namespace

bool SafeTensorsModelConfigReader::read(const std::filesystem::path &modelPath, ModelConfig &config)
{
    config = ModelConfig{};

    // 1. Confirm actual safetensors weights exist -- the one check
    // HfJsonModelConfigReader doesn't need, since it doesn't care which
    // weight-file format sits next to config.json.
    const auto singleFilePath = modelPath / "model.safetensors";
    const auto indexPath      = modelPath / "model.safetensors.index.json";

    const bool hasSingleFile   = std::filesystem::exists(singleFilePath);
    const bool hasShardedIndex = std::filesystem::exists(indexPath);

    if (!hasSingleFile && !hasShardedIndex) {
        JOB_LOG_ERROR("[SafeTensorsModelConfigReader] No model.safetensors or "
                      "model.safetensors.index.json found in: '{}'", modelPath.string());
        return false;
    }

    // 2. config.json remains the primary hyperparameter source -- tensor
    // shapes can't recover rope_theta, rms_norm_eps, or the architecture
    // name. Shared parsing with HfJsonModelConfigReader.
    const auto configPath = modelPath / "config.json";
    if (!std::filesystem::exists(configPath)) {
        JOB_LOG_ERROR("[SafeTensorsModelConfigReader] config.json not found in directory: '{}'", modelPath.string());
        return false;
    }

    bool blockCountKeyPresent = true;

    try {
        std::ifstream f(configPath);
        nlohmann::json j = nlohmann::json::parse(f);

        blockCountKeyPresent = j.contains("num_hidden_layers") && !j["num_hidden_layers"].is_null();

        io_util::populateModelConfigFromHfJson(j, modelPath.filename().string(), config);
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[SafeTensorsModelConfigReader] Failed to parse config.json: {}", e.what());
        return false;
    }

    // 3. Fallback-fill blockCount from tensor names when config.json didn't
    // have it. Layer count is the one field genuinely readable from names
    // alone ("model.layers.31...."), no shape-reading required -- most
    // other hyperparameters simply aren't recoverable this way.
    if (!blockCountKeyPresent) {
        const std::vector<std::string> tensorNames = hasSingleFile
                                                         ? readSafetensorsTensorNames(singleFilePath)
                                                         : readSafetensorsIndexTensorNames(indexPath);

        const uint32_t derivedBlockCount = io_util::estimateBlockCountFromTensorNames(tensorNames);
        if (derivedBlockCount > 0) {
            JOB_LOG_INFO("[SafeTensorsModelConfigReader] num_hidden_layers missing from config.json; "
                         "derived blockCount={} from tensor names", derivedBlockCount);
            config.transformerConfig().setBlockCount(derivedBlockCount);
        }
    }

    // 4. Optional Generation Parameters (SamplerConfig) -- same file, same
    // shared helper as HfJsonModelConfigReader.
    const auto genConfigPath = modelPath / "generation_config.json";
    if (std::filesystem::exists(genConfigPath)) {
        try {
            std::ifstream gf(genConfigPath);
            nlohmann::json gj = nlohmann::json::parse(gf);
            io_util::populateSamplerFromHfGenerationConfig(gj, config);
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[SafeTensorsModelConfigReader] Failed to parse generation_config.json (non-fatal): {}", e.what());
        }
    }

    if (!finalizeAndValidate(config, "SafeTensorsModelConfigReader"))
        return false;

    JOB_LOG_INFO("[SafeTensorsModelConfigReader] Successfully parsed config for '{}' "
                 "(Arch: {}, Layers: {}, Ctx: {}, Sharded: {})",
                 config.archConfig().modelName(), config.archConfig().archName(),
                 config.transformerConfig().blockCount(),
                 config.transformerConfig().contextLength(),
                 hasShardedIndex);

    return true;
}

} // namespace job::model