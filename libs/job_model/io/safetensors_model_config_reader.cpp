#include "safetensors_model_config_reader.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <job_logger.h>

#include "model_config_reader_utils.h"

namespace job::model {

uint32_t SafeTensorsModelConfigReader::estimateBlockCount(const std::vector<std::string> &tensorNames)
{
    int64_t maxIndex = -1;

    for (const auto &name : tensorNames) {
        for (const std::string_view marker : {".layers.", ".h."}) {
            auto pos = name.find(marker);
            if (pos == std::string::npos)
                continue;

            pos += marker.size();

            const auto end = name.find('.', pos);
            if (end == std::string::npos)
                continue;

            const std::string numStr = name.substr(pos, end - pos);
            if (numStr.empty() ||
                !std::all_of(numStr.begin(), numStr.end(),
                             [](unsigned char c) { return std::isdigit(c); }))
            {
                continue;
            }

            try {
                maxIndex = std::max(maxIndex, static_cast<int64_t>(std::stoll(numStr)));
            } catch (...) {
                // Ignore malformed or absurdly large layer indices.
            }
        }
    }

    return maxIndex >= 0 ? static_cast<uint32_t>(maxIndex + 1) : 0;
}


std::vector<std::string> SafeTensorsModelConfigReader::readTensorNames(const std::filesystem::path &path)
{
    std::vector<std::string> names;

    std::ifstream f(path, std::ios::binary);
    if (!f)
        return names;

    std::uint64_t headerLen = 0;
    f.read(reinterpret_cast<char *>(&headerLen), sizeof(headerLen));

    if (!f || headerLen == 0 || headerLen > (1ull << 30))
        return names;

    std::string headerBytes(headerLen, '\0');
    f.read(headerBytes.data(), static_cast<std::streamsize>(headerLen));

    if (!f)
        return names;

    try {
        const auto header = nlohmann::json::parse(headerBytes);

        for (auto it = header.begin(); it != header.end(); ++it) {
            if (it.key() != "__metadata__")
                names.push_back(it.key());
        }
    } catch (...) {
    }

    return names;
}

std::vector<std::string> SafeTensorsModelConfigReader::readIndexTensorNames(const std::filesystem::path &path)
{
    std::vector<std::string> names;

    std::ifstream f(path);
    if (!f)
        return names;

    try {
        const auto index = nlohmann::json::parse(f);

        if (index.contains("weight_map") && index["weight_map"].is_object()) {
            for (auto it = index["weight_map"].begin(); it != index["weight_map"].end(); ++it)
                names.push_back(it.key());
        }
    } catch (...) {
    }

    return names;
}
bool SafeTensorsModelConfigReader::read(const std::filesystem::path &modelPath, ModelConfig &config)
{
    config = ModelConfig{};

    const auto singleFilePath = modelPath / "model.safetensors";
    const auto indexPath      = modelPath / "model.safetensors.index.json";

    const bool hasSingleFile   = std::filesystem::is_regular_file(singleFilePath);
    const bool hasShardedIndex = std::filesystem::is_regular_file(indexPath);

    if (!hasSingleFile && !hasShardedIndex) {
        JOB_LOG_ERROR("[SafeTensorsModelConfigReader] No model.safetensors or "
                      "model.safetensors.index.json found in: '{}'", modelPath.string());
        return false;
    }

    const auto configPath = modelPath / "config.json";
    if (!std::filesystem::is_regular_file(configPath)) {
        JOB_LOG_ERROR("[SafeTensorsModelConfigReader] config.json not found in directory: '{}'", modelPath.string());
        return false;
    }

    try {
        std::ifstream stream{configPath};
        io_util::populateModelConfigFromHfJson(
            nlohmann::json::parse(stream),
            modelPath.filename().string(),
            config);
    } catch (const std::exception &e) {
        config = ModelConfig{};
        JOB_LOG_ERROR("[SafeTensorsModelConfigReader] Failed to parse config.json: {}", e.what());
        return false;
    }

    if (config.transformerConfig().blockCount() == 0) {
        const auto tensorNames = hasSingleFile
                                     ? readTensorNames(singleFilePath)
                                     : readIndexTensorNames(indexPath);

        const uint32_t blockCount = estimateBlockCount(tensorNames);
        if (blockCount > 0)
            config.transformerConfig().setBlockCount(blockCount);
    }

    const auto generationPath = modelPath / "generation_config.json";
    if (std::filesystem::is_regular_file(generationPath)) {
        try {
            std::ifstream stream{generationPath};
            io_util::populateSamplerFromHfGenerationConfig(nlohmann::json::parse(stream), config);
        } catch (const std::exception &e) {
            JOB_LOG_ERROR("[SafeTensorsModelConfigReader] Failed to parse generation_config.json (non-fatal): {}", e.what());
        }
    }

    return finalizeAndValidate(config, "SafeTensorsModelConfigReader");
}

} // namespace job::model