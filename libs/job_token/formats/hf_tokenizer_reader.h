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

#include "jobtoken_export.h"

namespace job::token {

enum class HfModelType : uint8_t {
    Unknown = 0,
    BPE,
    WordPiece,
    Unigram,
    WordLevel
};

[[nodiscard]] JOBTOKEN_EXPORT constexpr std::string_view hfModelTypeToString(HfModelType type) noexcept
{
    switch (type) {
    case HfModelType::BPE:       return "BPE";
    case HfModelType::WordPiece: return "WordPiece";
    case HfModelType::Unigram:   return "Unigram";
    case HfModelType::WordLevel: return "WordLevel";
    case HfModelType::Unknown:
    default:                     return "Unknown";
    }
}

[[nodiscard]] JOBTOKEN_EXPORT constexpr HfModelType stringToHfModelType(std::string_view str) noexcept
{
    if (str == "BPE")       return HfModelType::BPE;
    if (str == "WordPiece") return HfModelType::WordPiece;
    if (str == "Unigram")   return HfModelType::Unigram;
    if (str == "WordLevel") return HfModelType::WordLevel;
    return HfModelType::Unknown;
}

struct JOBTOKEN_EXPORT HfAddedToken {
    int32_t     id{-1};
    std::string content;
    bool        special{false};
    bool        singleWord{false};
    bool        lstrip{false};
    bool        rstrip{false};
    bool        normalized{false};
};

struct JOBTOKEN_EXPORT HfTokenizerData {
    HfModelType                                modelType{HfModelType::BPE};
    std::vector<std::pair<std::string, float>> vocab;           // Token string and score/rank
    std::unordered_map<std::string, int32_t>   tokenToId;       // Fast token-to-ID lookup
    std::vector<std::pair<std::string, std::string>> merges;    // (Left, Right) pairs for BPE
    std::vector<HfAddedToken>                  addedTokens;     // Special / added tokens list

    // Special token identities
    std::string unkToken;
    std::string bosToken;
    std::string eosToken;
    std::string padToken;
    std::string clsToken;
    std::string sepToken;
    std::string maskToken;

    // Jinja chat template if defined
    std::string chatTemplate;

    // Pre-tokenizer / normalizer settings
    bool byteFallback{false};
    bool addPrefixSpace{false};
    bool cleanUpTokenizationSpaces{true};
    bool addBosToken{false};
    bool addEosToken{false};
};

class JOBTOKEN_EXPORT HfTokenizerReader {
public:
    HfTokenizerReader() = default;
    ~HfTokenizerReader() = default;

    // Load from filesystem
    [[nodiscard]] bool loadFromFile(
        const std::filesystem::path& tokenizerJsonPath,
        const std::filesystem::path& tokenizerConfigJsonPath = {});

    // Load from memory buffers
    [[nodiscard]] bool loadFromMemory(
        std::string_view tokenizerJson,
        std::string_view tokenizerConfigJson = {});

    // Individual file/buffer loaders
    [[nodiscard]] bool loadTokenizerJson(std::string_view jsonContent);
    [[nodiscard]] bool loadTokenizerConfigFile(const std::filesystem::path& configPath);
    [[nodiscard]] bool loadTokenizerConfigJson(std::string_view jsonContent);

    // Accessors
    [[nodiscard]] const HfTokenizerData& data() const noexcept { return m_data; }
    [[nodiscard]] HfTokenizerData& data() noexcept { return m_data; }

    [[nodiscard]] HfModelType modelType() const noexcept { return m_data.modelType; }
    [[nodiscard]] size_t vocabSize() const noexcept { return m_data.vocab.size(); }
    [[nodiscard]] const std::string& chatTemplate() const noexcept { return m_data.chatTemplate; }

    // Special token accessors
    [[nodiscard]] const std::string& bosToken() const noexcept { return m_data.bosToken; }
    [[nodiscard]] const std::string& eosToken() const noexcept { return m_data.eosToken; }
    [[nodiscard]] const std::string& unkToken() const noexcept { return m_data.unkToken; }
    [[nodiscard]] const std::string& padToken() const noexcept { return m_data.padToken; }
    [[nodiscard]] const std::string& clsToken() const noexcept { return m_data.clsToken; }
    [[nodiscard]] const std::string& sepToken() const noexcept { return m_data.sepToken; }
    [[nodiscard]] const std::string& maskToken() const noexcept { return m_data.maskToken; }

    [[nodiscard]] std::optional<int32_t> findTokenId(std::string_view token) const noexcept;
    [[nodiscard]] std::optional<std::string_view> findTokenString(int32_t id) const noexcept;

    void clear() noexcept;

private:
    HfTokenizerData m_data;
};

} // namespace job::token