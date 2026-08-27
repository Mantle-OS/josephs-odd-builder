#pragma once

#include <cstdint>
#include <string_view>

#include "jobmodel_export.h"

namespace job::model {

enum class ModelArchitecture : uint16_t {
    Unknown = 0,
    Llama,        // LLaMA, LLaMA-2, LLaMA-3, Mistral, Mixtral, Vicuna
    Gemma,        // Gemma, CodeGemma
    Gemma2,       // Gemma-2
    Qwen,         // Qwen, Qwen-1.5
    Qwen2,        // Qwen-2, Qwen-2.5
    Qwen2Moe,     // Qwen-2 MoE
    Qwen3,        // Qwen3 example
    Phi2,         // Phi-2
    Phi3,         // Phi-3, Phi-3.5
    StarCoder2,   // StarCoder-2
    Falcon,       // Falcon-7B, Falcon-40B
    DeepSeek,     // DeepSeek-V1, DeepSeek-V2, DeepSeek-V3
    Bert,         // BERT, Nomic-BERT
    CommandR      // Cohere Command-R / Command-R+
};

enum class RopeScalingType : uint8_t {
    None = 0,
    Linear,
    Yarn,
    Dynamic,
    LongRoPE
};

enum class PoolingType : uint8_t {
    None = 0,
    Mean,
    Cls,
    Last
};

[[nodiscard]] JOBMODEL_EXPORT constexpr std::string_view modelArchitectureToString(ModelArchitecture arch) noexcept
{
    switch (arch) {
    case ModelArchitecture::Llama:
        return "llama";
    case ModelArchitecture::Gemma:
        return "gemma";
    case ModelArchitecture::Gemma2:
        return "gemma2";
    case ModelArchitecture::Qwen:
        return "qwen";
    case ModelArchitecture::Qwen2:
        return "qwen2";
    case ModelArchitecture::Qwen3:
        return "qwen3";
    case ModelArchitecture::Qwen2Moe:
        return "qwen2moe";
    case ModelArchitecture::Phi2:
        return "phi2";
    case ModelArchitecture::Phi3:
        return "phi3";
    case ModelArchitecture::StarCoder2:
        return "starcoder2";
    case ModelArchitecture::Falcon:
        return "falcon";
    case ModelArchitecture::DeepSeek:
        return "deepseek";
    case ModelArchitecture::Bert:
        return "bert";
    case ModelArchitecture::CommandR:
        return "command-r";
    case ModelArchitecture::Unknown:
    default:
        return "unknown";
    }
}

[[nodiscard]] JOBMODEL_EXPORT constexpr ModelArchitecture stringToModelArchitecture(std::string_view str) noexcept
{
    if (str == "llama" || str == "mistral" || str == "mixtral")
        return ModelArchitecture::Llama;
    if (str == "gemma")
        return ModelArchitecture::Gemma;
    if (str == "gemma2")
        return ModelArchitecture::Gemma2;
    if (str == "qwen")
        return ModelArchitecture::Qwen;
    if (str == "qwen2")
        return ModelArchitecture::Qwen2;
    if (str == "qwen2moe")
        return ModelArchitecture::Qwen2Moe;
    if (str == "qwen3")
        return ModelArchitecture::Qwen3;
    if (str == "phi2" || str == "phi-2")
        return ModelArchitecture::Phi2;
    if (str == "phi3" || str == "phi-3")
        return ModelArchitecture::Phi3;
    if (str == "starcoder2")
        return ModelArchitecture::StarCoder2;
    if (str == "falcon")
        return ModelArchitecture::Falcon;
    if (str == "deepseek" || str == "deepseek2")
        return ModelArchitecture::DeepSeek;
    if (str == "bert" || str == "nomic-bert")
        return ModelArchitecture::Bert;
    if (str == "command-r" || str == "cohere")
        return ModelArchitecture::CommandR;
    return ModelArchitecture::Unknown;
}

} // namespace job::model