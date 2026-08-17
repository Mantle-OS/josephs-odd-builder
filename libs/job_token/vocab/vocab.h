#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "job_token_types.h"
#include "token_record.h"
#include "special_tokens.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT Vocab
{
public:
    using Ptr  = std::shared_ptr<Vocab>;
    using WPtr = std::weak_ptr<Vocab>;
    using UPtr = std::unique_ptr<Vocab>;

    Vocab() = default;
    ~Vocab() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<Vocab>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<Vocab>();
    }

    Vocab(const Vocab &) = default;
    Vocab &operator=(const Vocab &) = default;
    Vocab(Vocab &&) noexcept = default;
    Vocab &operator=(Vocab &&) noexcept = default;

    TokenId addToken(std::string text, float score = 0.0f, StructuralType type = StructuralType::Normal)
    {
        if (text.empty())
            return kInvalidToken;

        const TokenId id = static_cast<TokenId>(m_records.size());

        TokenRecord record;
        record.setId(id);
        record.setText(std::move(text));
        record.setScore(score);
        record.setType(type);

        m_textToId[record.text()] = id;
        m_records.push_back(std::move(record));

        return id;
    }

    void setToken(TokenId id, std::string text, float score = 0.0f, StructuralType type = StructuralType::Normal)
    {
        if (id < 0 || text.empty())
            return;

        const size_t index = static_cast<size_t>(id);

        if (index >= m_records.size())
            m_records.resize(index + 1);

        TokenRecord &record = m_records[index];

        if (record.isValid() && record.text() != text) {
            auto oldIt = m_textToId.find(record.text());

            if (oldIt != m_textToId.end() && oldIt->second == id)
                m_textToId.erase(oldIt);
        }

        record.setId(id);
        record.setText(std::move(text));
        record.setScore(score);
        record.setType(type);

        m_textToId[record.text()] = id;
    }

    void setTokenScore(TokenId id, float score) noexcept
    {
        TokenRecord *token = record(id);
        if (token)
            token->setScore(score);
    }

    void setTokenType(TokenId id, StructuralType type) noexcept
    {
        TokenRecord *token = record(id);
        if (token)
            token->setType(type);
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

    [[nodiscard]] TokenId findId(std::string_view text) const noexcept
    {
        auto it = m_textToId.find(text);

        if (it != m_textToId.end())
            return it->second;

        return kInvalidToken;
    }

    [[nodiscard]] std::string_view tokenText(TokenId id) const noexcept
    {
        const TokenRecord *token = record(id);

        if (!token)
            return {};

        return token->text();
    }

    [[nodiscard]] float tokenScore(TokenId id) const noexcept
    {
        const TokenRecord *token = record(id);

        if (!token)
            return 0.0f;

        return token->score();
    }

    [[nodiscard]] StructuralType tokenType(TokenId id) const noexcept
    {
        const TokenRecord *token = record(id);

        if (!token)
            return StructuralType::Unknown;

        return token->type();
    }

    [[nodiscard]] TokenRecord *record(TokenId id) noexcept
    {
        if (id < 0)
            return nullptr;

        const size_t index = static_cast<size_t>(id);

        if (index >= m_records.size())
            return nullptr;

        if (!m_records[index].isValid())
            return nullptr;

        return &m_records[index];
    }

    [[nodiscard]] const TokenRecord *record(TokenId id) const noexcept
    {
        if (id < 0)
            return nullptr;

        const size_t index = static_cast<size_t>(id);

        if (index >= m_records.size())
            return nullptr;

        if (!m_records[index].isValid())
            return nullptr;

        return &m_records[index];
    }

    [[nodiscard]] size_t size() const noexcept
    {
        return m_records.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_records.empty();
    }

    [[nodiscard]] SpecialTokens &specialTokens() noexcept
    {
        return m_special;
    }

    [[nodiscard]] const SpecialTokens &specialTokens() const noexcept
    {
        return m_special;
    }

    [[nodiscard]] std::span<TokenRecord> records() noexcept
    {
        return m_records;
    }

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