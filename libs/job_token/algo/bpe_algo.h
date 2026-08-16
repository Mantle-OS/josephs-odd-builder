#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>
#include <limits>
#include <string_view>
#include <span>
#include <memory>

#include "algo/itoken_algo.h"
#include "core/byte_fallback.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT BpeAlgo final : public ITokenAlgo {
public:
    using Ptr  = std::shared_ptr<BpeAlgo>;
    using UPtr = std::unique_ptr<BpeAlgo>;

    static constexpr uint32_t kInvalidRank = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t kInvalidId   = std::numeric_limits<uint32_t>::max();

    struct MergeRule {
        TokenId left{kInvalidToken};
        TokenId right{kInvalidToken};
        TokenId newId{kInvalidToken};
    };

    explicit BpeAlgo(Vocab::Ptr vocab);
    ~BpeAlgo() override = default;

    [[nodiscard]] static UPtr create(Vocab::Ptr vocab)
    {
        return std::make_unique<BpeAlgo>(std::move(vocab));
    }

    [[nodiscard]] TokenizerAlgorithm type() const noexcept override
    {
        return TokenizerAlgorithm::BPE;
    }

    void addMergeRule(TokenId left, TokenId right, TokenId newId);
    void setMergeRules(std::vector<MergeRule> rules);
    void clearRules() noexcept;

    size_t encodeChunk(std::string_view chunk, std::span<TokenId> outTokens) const override;
    size_t decodeTokens(std::span<const TokenId> tokens, std::span<char> outBuffer) const override;

    [[nodiscard]] std::span<const MergeRule> rules() const noexcept
    {
        return m_rules;
    }

private:
    struct Symbol {
        uint32_t prev = kInvalidId;
        uint32_t next = kInvalidId;
        TokenId  id   = kInvalidToken;
    };

    struct QueueElement {
        uint32_t pos;
        uint32_t rank;
        TokenId  leftId;
        TokenId  rightId;

        bool operator>(const QueueElement& o) const noexcept
        {
            if (rank != o.rank) return rank > o.rank;
            return pos > o.pos;
        }
    };

    [[nodiscard]] static constexpr uint64_t makePairKey(TokenId left, TokenId right) noexcept
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(left)) << 32) |
               static_cast<uint64_t>(static_cast<uint32_t>(right));
    }

    void rebuildAccelerators();
    [[nodiscard]] TokenId findInitialByteToken(uint8_t byteVal) const noexcept;

    std::vector<MergeRule>                  m_rules;
    std::unordered_map<uint64_t, uint32_t>  m_pairToRank;
    std::array<uint32_t, 256 * 256>         m_bytePairRank{};
};

} // namespace job::token