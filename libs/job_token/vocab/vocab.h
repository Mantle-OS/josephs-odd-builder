#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <span>
#include <memory>

#include "job_tokenizer_types.h"
#include "token_record.h"
#include "special_tokens.h"
#include "jobtoken_export.h"

namespace job::token {

// Transparent string hash helper for C++20 heterogeneous string_view lookups
struct StringHash {
    using is_transparent = void;

    [[nodiscard]] size_t operator()(std::string_view sv) const noexcept
    {
        return std::hash<std::string_view>{}(sv);
    }
};

class JOBTOKEN_EXPORT Vocab {
public:
    using Ptr  = std::shared_ptr<Vocab>;
    using UPtr = std::unique_ptr<Vocab>;

    Vocab() = default;
    ~Vocab() = default;

    Vocab(const Vocab&) = default;
    Vocab& operator=(const Vocab&) = default;
    Vocab(Vocab&&) noexcept = default;
    Vocab& operator=(Vocab&&) noexcept = default;

    [[nodiscard]] static Ptr  createShared() { return std::make_shared<Vocab>(); }
    [[nodiscard]] static UPtr createUniq()   { return std::make_unique<Vocab>(); }

    // --- Insertion & Construction ---

    TokenId addToken(std::string text, float score = 0.0f, TokenType type = TokenType::Normal)
    {
        const TokenId id = static_cast<TokenId>(m_records.size());
        m_records.push_back({text, id, score, type});
        m_textToId[m_records.back().text] = id;
        return id;
    }

    void setToken(TokenId id, std::string text, float score = 0.0f, TokenType type = TokenType::Normal)
    {
        if (id < 0)
            return;
        const size_t idx = static_cast<size_t>(id);
        if (idx >= m_records.size())
            m_records.resize(idx + 1);

        m_records[idx] = {text, id, score, type};
        m_textToId[m_records[idx].text] = id;
    }

    void reserve(size_t capacity)
    {
        m_records.reserve(capacity);
        m_textToId.reserve(capacity);
    }

    void clear() noexcept
    {
        m_records.clear();
        m_textToId.clear();
        m_special.reset();
    }

    // --- Fast Queries ---

    [[nodiscard]] TokenId findId(std::string_view text) const noexcept
    {
        auto it = m_textToId.find(text);
        if (it != m_textToId.end())
            return it->second;
        return kInvalidToken;
    }

    [[nodiscard]] std::string_view tokenText(TokenId id) const noexcept
    {
        if (id < 0 || static_cast<size_t>(id) >= m_records.size())
            return {};
        return m_records[static_cast<size_t>(id)].text;
    }

    [[nodiscard]] float tokenScore(TokenId id) const noexcept
    {
        if (id < 0 || static_cast<size_t>(id) >= m_records.size())
            return 0.0f;
        return m_records[static_cast<size_t>(id)].score;
    }

    [[nodiscard]] TokenType tokenType(TokenId id) const noexcept
    {
        if (id < 0 || static_cast<size_t>(id) >= m_records.size())
            return TokenType::Unknown;
        return m_records[static_cast<size_t>(id)].type;
    }

    [[nodiscard]] const TokenRecord* record(TokenId id) const noexcept
    {
        if (id < 0 || static_cast<size_t>(id) >= m_records.size())
            return nullptr;
        return &m_records[static_cast<size_t>(id)];
    }

    // --- Vocabulary State & Special Tokens ---

    [[nodiscard]] size_t size() const noexcept { return m_records.size(); }
    [[nodiscard]] bool empty() const noexcept { return m_records.empty(); }

    [[nodiscard]] SpecialTokens& specialTokens() noexcept { return m_special; }
    [[nodiscard]] const SpecialTokens& specialTokens() const noexcept { return m_special; }

    [[nodiscard]] std::span<const TokenRecord> records() const noexcept
    {
        return m_records;
    }

private:
    std::vector<TokenRecord> m_records;
    std::unordered_map<std::string, TokenId, StringHash, std::equal_to<>> m_textToId;
    SpecialTokens m_special;
};

} // namespace job::token