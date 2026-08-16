#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "formats/hf_tokenizer_reader.h"
#include "job_gguf.h"
#include "jobtoken_export.h"

namespace job::token {

enum class GgufTokenType : uint8_t {
    Normal      = 1,
    Unknown     = 2,
    Control     = 3,
    UserDefined = 4,
    Unused      = 5,
    Byte        = 6
};

struct JOBTOKEN_EXPORT GgufTokenEntry {
    std::string   text;
    float         score{0.0f};
    GgufTokenType type{GgufTokenType::Normal};
};

struct JOBTOKEN_EXPORT GgufTokenizerData {
    std::string modelName;       // e.g., "llama", "gpt2", "bert", "t5"
    HfModelType modelType{HfModelType::BPE};
    std::string preTokenizer;    // e.g., "llama-v3", "qwen2", "default"

    // Vocab & Merges
    std::vector<GgufTokenEntry>                      vocab;
    std::unordered_map<std::string, int32_t>         tokenToId;
    std::vector<std::pair<std::string, std::string>> merges;

    // Special token IDs (-1 if unassigned)
    int32_t bosId{-1};
    int32_t eosId{-1};
    int32_t unkId{-1};
    int32_t padId{-1};
    int32_t clsId{-1};
    int32_t sepId{-1};
    int32_t maskId{-1};

    // Resolved special token strings
    std::string bosToken;
    std::string eosToken;
    std::string unkToken;
    std::string padToken;

    // Jinja chat template if present
    std::string chatTemplate;

    // Preprocessing flags
    bool addBosToken{true};
    bool addEosToken{false};
    bool addPrefixSpace{false};
};

class JOBTOKEN_EXPORT GgufTokenizerReader {
public:
    GgufTokenizerReader() = default;
    ~GgufTokenizerReader() = default;

    // Load directly from GGUF file on disk
    [[nodiscard]] bool loadFromFile(const std::filesystem::path& path);

    // Load from memory buffers
    [[nodiscard]] bool loadFromMemory(const void* data, size_t size);
    [[nodiscard]] bool loadFromMemory(std::span<const std::byte> buffer);

    // Load from an already opened JobGguf instance
    [[nodiscard]] bool loadFromGguf(const ggml::JobGguf& gguf);

    // Accessors
    [[nodiscard]] const GgufTokenizerData& data() const noexcept { return m_data; }
    [[nodiscard]] GgufTokenizerData& data() noexcept { return m_data; }

    [[nodiscard]] size_t vocabSize() const noexcept { return m_data.vocab.size(); }
    [[nodiscard]] const std::string& modelName() const noexcept { return m_data.modelName; }
    [[nodiscard]] HfModelType modelType() const noexcept { return m_data.modelType; }
    [[nodiscard]] const std::string& chatTemplate() const noexcept { return m_data.chatTemplate; }

    [[nodiscard]] int32_t bosId() const noexcept { return m_data.bosId; }
    [[nodiscard]] int32_t eosId() const noexcept { return m_data.eosId; }
    [[nodiscard]] int32_t unkId() const noexcept { return m_data.unkId; }
    [[nodiscard]] int32_t padId() const noexcept { return m_data.padId; }

    [[nodiscard]] const std::string& bosToken() const noexcept { return m_data.bosToken; }
    [[nodiscard]] const std::string& eosToken() const noexcept { return m_data.eosToken; }
    [[nodiscard]] const std::string& unkToken() const noexcept { return m_data.unkToken; }
    [[nodiscard]] const std::string& padToken() const noexcept { return m_data.padToken; }

    [[nodiscard]] std::optional<int32_t> findTokenId(std::string_view token) const noexcept;
    [[nodiscard]] std::optional<std::string_view> findTokenString(int32_t id) const noexcept;

    void clear() noexcept;

private:
    GgufTokenizerData m_data;
};

} // namespace job::token