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
#include "jobtoken_export.h"

namespace job::token {

enum class BinaryTokenType : uint8_t {
    Normal = 0,
    Special,
    Control,
    Byte,
    Unused
};

struct JOBTOKEN_EXPORT BinaryTokenEntry {
    std::string     content;
    float           score{0.0f};
    BinaryTokenType tokenType{BinaryTokenType::Normal};
};

struct JOBTOKEN_EXPORT BinaryVocabData {
    uint32_t    version{1};
    HfModelType modelType{HfModelType::BPE};
    bool        byteFallback{false};
    bool        addPrefixSpace{false};

    // Special token IDs (-1 if unassigned)
    int32_t bosId{-1};
    int32_t eosId{-1};
    int32_t unkId{-1};
    int32_t padId{-1};
    int32_t maskId{-1};

    // Jinja chat template if present
    std::string chatTemplate;

    // Vocab and lookup tables
    std::vector<BinaryTokenEntry>            vocab;
    std::unordered_map<std::string, int32_t> tokenToId;
    std::vector<std::pair<std::string, std::string>> merges;
};

class JOBTOKEN_EXPORT BinaryVocabReader {
public:
    static constexpr uint32_t MAGIC = 0x4A4F4256; // 'J' 'O' 'B' 'V'
    static constexpr uint32_t CURRENT_VERSION = 1;

    BinaryVocabReader() = default;
    ~BinaryVocabReader() = default;

    // Load from filesystem
    [[nodiscard]] bool loadFromFile(const std::filesystem::path& path);

    // Load from memory buffers
    [[nodiscard]] bool loadFromMemory(std::span<const uint8_t> buffer);
    [[nodiscard]] bool loadFromMemory(const void* data, size_t size);

    // Accessors
    [[nodiscard]] const BinaryVocabData& data() const noexcept { return m_data; }
    [[nodiscard]] BinaryVocabData& data() noexcept { return m_data; }

    [[nodiscard]] size_t vocabSize() const noexcept { return m_data.vocab.size(); }
    [[nodiscard]] HfModelType modelType() const noexcept { return m_data.modelType; }
    [[nodiscard]] const std::string& chatTemplate() const noexcept { return m_data.chatTemplate; }

    [[nodiscard]] int32_t bosId() const noexcept { return m_data.bosId; }
    [[nodiscard]] int32_t eosId() const noexcept { return m_data.eosId; }
    [[nodiscard]] int32_t unkId() const noexcept { return m_data.unkId; }
    [[nodiscard]] int32_t padId() const noexcept { return m_data.padId; }

    [[nodiscard]] std::optional<int32_t> findTokenId(std::string_view token) const noexcept;
    [[nodiscard]] std::optional<std::string_view> findTokenString(int32_t id) const noexcept;

    void clear() noexcept;

private:
    BinaryVocabData m_data;
};

} // namespace job::token