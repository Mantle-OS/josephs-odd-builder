#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <optional>

#include "job_tokenizer_types.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT SpecialTokens {
public:
    SpecialTokens() = default;
    ~SpecialTokens() = default;

    SpecialTokens(const SpecialTokens&) = default;
    SpecialTokens& operator=(const SpecialTokens&) = default;
    SpecialTokens(SpecialTokens&&) noexcept = default;
    SpecialTokens& operator=(SpecialTokens&&) noexcept = default;

    // --- Standard Canonical Accessors ---
    [[nodiscard]] TokenId bosId() const noexcept { return m_bosId; }
    [[nodiscard]] TokenId eosId() const noexcept { return m_eosId; }
    [[nodiscard]] TokenId eotId() const noexcept { return m_eotId; }
    [[nodiscard]] TokenId padId() const noexcept { return m_padId; }
    [[nodiscard]] TokenId unkId() const noexcept { return m_unkId; }
    [[nodiscard]] TokenId maskId() const noexcept { return m_maskId; }
    [[nodiscard]] TokenId prefixId() const noexcept { return m_prefixId; }
    [[nodiscard]] TokenId suffixId() const noexcept { return m_suffixId; }
    [[nodiscard]] TokenId middleId() const noexcept { return m_middleId; }

    void setBosId(TokenId id) noexcept { m_bosId = id; registerSpecialId(id); }
    void setEosId(TokenId id) noexcept { m_eosId = id; registerSpecialId(id); }
    void setEotId(TokenId id) noexcept { m_eotId = id; registerSpecialId(id); }
    void setPadId(TokenId id) noexcept { m_padId = id; registerSpecialId(id); }
    void setUnkId(TokenId id) noexcept { m_unkId = id; registerSpecialId(id); }
    void setMaskId(TokenId id) noexcept { m_maskId = id; registerSpecialId(id); }
    void setPrefixId(TokenId id) noexcept { m_prefixId = id; registerSpecialId(id); }
    void setSuffixId(TokenId id) noexcept { m_suffixId = id; registerSpecialId(id); }
    void setMiddleId(TokenId id) noexcept { m_middleId = id; registerSpecialId(id); }

    // --- Custom / Named Special Tokens Registration ---
    void registerSpecial(std::string name, TokenId id, SpecialTokenType type = SpecialTokenType::None)
    {
        if (id == kInvalidToken || name.empty())
            return;

        m_nameToId[name] = id;
        m_idToName[id] = name;
        registerSpecialId(id);

        switch (type) {
        case SpecialTokenType::Bos:    m_bosId = id;    break;
        case SpecialTokenType::Eos:    m_eosId = id;    break;
        case SpecialTokenType::Eot:    m_eotId = id;    break;
        case SpecialTokenType::Pad:    m_padId = id;    break;
        case SpecialTokenType::Unk:    m_unkId = id;    break;
        case SpecialTokenType::Mask:   m_maskId = id;   break;
        case SpecialTokenType::Prefix: m_prefixId = id; break;
        case SpecialTokenType::Suffix: m_suffixId = id; break;
        case SpecialTokenType::Middle: m_middleId = id; break;
        case SpecialTokenType::None:   break;
        }
    }

    [[nodiscard]] bool isSpecial(TokenId id) const noexcept
    {
        if (id == kInvalidToken)
            return false;
        return m_specialIds.contains(id);
    }

    [[nodiscard]] std::optional<TokenId> findByName(std::string_view name) const noexcept
    {
        auto it = m_nameToId.find(std::string(name));
        if (it != m_nameToId.end())
            return it->second;
        return std::nullopt;
    }

    [[nodiscard]] std::string_view nameById(TokenId id) const noexcept
    {
        auto it = m_idToName.find(id);
        if (it != m_idToName.end())
            return it->second;
        return {};
    }

    void reset() noexcept
    {
        m_bosId = kInvalidToken;
        m_eosId = kInvalidToken;
        m_eotId = kInvalidToken;
        m_padId = kInvalidToken;
        m_unkId = kInvalidToken;
        m_maskId = kInvalidToken;
        m_prefixId = kInvalidToken;
        m_suffixId = kInvalidToken;
        m_middleId = kInvalidToken;
        m_nameToId.clear();
        m_idToName.clear();
        m_specialIds.clear();
    }

private:
    void registerSpecialId(TokenId id) noexcept
    {
        if (id != kInvalidToken)
            m_specialIds.insert(id);
    }

    TokenId m_bosId{kInvalidToken};
    TokenId m_eosId{kInvalidToken};
    TokenId m_eotId{kInvalidToken};
    TokenId m_padId{kInvalidToken};
    TokenId m_unkId{kInvalidToken};
    TokenId m_maskId{kInvalidToken};
    TokenId m_prefixId{kInvalidToken};
    TokenId m_suffixId{kInvalidToken};
    TokenId m_middleId{kInvalidToken};

    std::unordered_map<std::string, TokenId> m_nameToId;
    std::unordered_map<TokenId, std::string> m_idToName;
    std::unordered_set<TokenId>              m_specialIds;
};

} // namespace job::token