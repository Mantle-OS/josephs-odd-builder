#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include <job_hash_container.h>
#include <job_logger.h>

#include "job_token_enums.h"
#include "job_token_types.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT SpecialTokens
{
public:
    using Ptr  = std::shared_ptr<SpecialTokens>;
    using WPtr = std::weak_ptr<SpecialTokens>;
    using UPtr = std::unique_ptr<SpecialTokens>;

    SpecialTokens() = default;
    ~SpecialTokens() = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<SpecialTokens>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<SpecialTokens>();
    }

    SpecialTokens(const SpecialTokens &) = default;
    SpecialTokens &operator=(const SpecialTokens &) = default;
    SpecialTokens(SpecialTokens &&) noexcept = default;
    SpecialTokens &operator=(SpecialTokens &&) noexcept = default;

    [[nodiscard]] TokenId bosId() const noexcept { return m_bosId; }
    [[nodiscard]] TokenId eosId() const noexcept { return m_eosId; }
    [[nodiscard]] TokenId eotId() const noexcept { return m_eotId; }
    [[nodiscard]] TokenId padId() const noexcept { return m_padId; }
    [[nodiscard]] TokenId unkId() const noexcept { return m_unkId; }
    [[nodiscard]] TokenId maskId() const noexcept { return m_maskId; }
    [[nodiscard]] TokenId prefixId() const noexcept { return m_prefixId; }
    [[nodiscard]] TokenId suffixId() const noexcept { return m_suffixId; }
    [[nodiscard]] TokenId middleId() const noexcept { return m_middleId; }
    [[nodiscard]] TokenId clsId() const noexcept { return m_clsId; }
    [[nodiscard]] TokenId sepId() const noexcept { return m_sepId; }

    void setBosId(TokenId id) noexcept { setCanonicalId(m_bosId, id); }
    void setEosId(TokenId id) noexcept { setCanonicalId(m_eosId, id); }
    void setEotId(TokenId id) noexcept { setCanonicalId(m_eotId, id); }
    void setPadId(TokenId id) noexcept { setCanonicalId(m_padId, id); }
    void setUnkId(TokenId id) noexcept { setCanonicalId(m_unkId, id); }
    void setMaskId(TokenId id) noexcept { setCanonicalId(m_maskId, id); }
    void setPrefixId(TokenId id) noexcept { setCanonicalId(m_prefixId, id); }
    void setSuffixId(TokenId id) noexcept { setCanonicalId(m_suffixId, id); }
    void setMiddleId(TokenId id) noexcept { setCanonicalId(m_middleId, id); }  
    void setClsId(TokenId id) noexcept { setCanonicalId(m_clsId, id); }
    void setSepId(TokenId id) noexcept { setCanonicalId(m_sepId, id); }


    void registerSpecial(std::string name, TokenId id, SpecialTokenType type = SpecialTokenType::None)
    {
        if (id == kInvalidToken || name.empty())
            return;

        auto nameIt = m_nameToId.find(name);
        if (nameIt != m_nameToId.end() && nameIt->second != id) {
            const TokenId oldId = nameIt->second;

            if (!m_idToName.remove(oldId)) {
                JOB_LOG_ASSERT(
                    "SpecialTokens invariant failure: name '{}' maps to token {}, "
                    "but reverse token-to-name mapping does not exist",
                    name,
                    oldId);
                return;
            }

            if (!isCanonicalId(oldId))
                m_specialIds.erase(oldId);
        }

        auto idIt = m_idToName.find(id);
        if (idIt != m_idToName.end() && idIt->second != name) {
            const std::string oldName = idIt->second;

            if (!m_nameToId.remove(oldName)) {
                JOB_LOG_ASSERT(
                    "SpecialTokens invariant failure: token {} maps to name '{}', "
                    "but reverse name-to-token mapping does not exist",
                    id,
                    oldName);
                return;
            }
        }

        m_nameToId[name] = id;
        m_idToName[id] = std::move(name);
        m_specialIds.insert(id);

        switch (type) {
        case SpecialTokenType::Bos:
            setBosId(id);
            break;
        case SpecialTokenType::Eos:
            setEosId(id);
            break;
        case SpecialTokenType::Eot:
            setEotId(id);
            break;
        case SpecialTokenType::Pad:
            setPadId(id);
            break;
        case SpecialTokenType::Unk:
            setUnkId(id);
            break;
        case SpecialTokenType::Mask:
            setMaskId(id);
            break;
        case SpecialTokenType::Prefix:
            setPrefixId(id);
            break;
        case SpecialTokenType::Suffix:
            setSuffixId(id);
            break;
        case SpecialTokenType::Middle:
            setMiddleId(id);
            break;
        case SpecialTokenType::None:
            break;
        case SpecialTokenType::Cls:
            setClsId(id);
            break;
        case SpecialTokenType::Sep:
            setSepId(id);
            break;
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
        m_bosId     = kInvalidToken;
        m_eosId     = kInvalidToken;
        m_eotId     = kInvalidToken;
        m_padId     = kInvalidToken;
        m_unkId     = kInvalidToken;
        m_maskId    = kInvalidToken;
        m_prefixId  = kInvalidToken;
        m_suffixId  = kInvalidToken;
        m_middleId  = kInvalidToken;
        m_clsId     = kInvalidToken;
        m_sepId     = kInvalidToken;

        m_nameToId.clear();
        m_idToName.clear();
        m_specialIds.clear();
    }

private:
    [[nodiscard]] bool isCanonicalId(TokenId id) const noexcept
    {
        return m_bosId      == id ||
               m_eosId      == id ||
               m_eotId      == id ||
               m_padId      == id ||
               m_unkId      == id ||
               m_maskId     == id ||
               m_prefixId   == id ||
               m_suffixId   == id ||
               m_middleId   == id ||
               m_clsId      == id ||
               m_sepId      == id;
    }

    void setCanonicalId(TokenId &target, TokenId id) noexcept
    {
        const TokenId oldId = target;
        target = id;

        if (id != kInvalidToken)
            m_specialIds.insert(id);

        if (oldId != kInvalidToken &&
            !isCanonicalId(oldId) &&
            !m_idToName.contains(oldId)) {
            m_specialIds.erase(oldId);
        }
    }

    TokenId                             m_bosId{kInvalidToken};
    TokenId                             m_eosId{kInvalidToken};
    TokenId                             m_eotId{kInvalidToken};
    TokenId                             m_padId{kInvalidToken};
    TokenId                             m_unkId{kInvalidToken};
    TokenId                             m_maskId{kInvalidToken};
    TokenId                             m_prefixId{kInvalidToken};
    TokenId                             m_suffixId{kInvalidToken};
    TokenId                             m_middleId{kInvalidToken};
    TokenId                             m_clsId{kInvalidToken};
    TokenId                             m_sepId{kInvalidToken};
    core::JobHash<std::string, TokenId> m_nameToId;
    core::JobHash<TokenId, std::string> m_idToName;
    std::unordered_set<TokenId>         m_specialIds;
};

} // namespace job::token