#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "itoken_algo.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT Bpe final : public ITokenAlgo
{
public:
    using Ptr  = std::shared_ptr<Bpe>;
    using WPtr = std::weak_ptr<Bpe>;
    using UPtr = std::unique_ptr<Bpe>;

    static constexpr std::uint32_t kInvalidRank = std::numeric_limits<std::uint32_t>::max();
    static constexpr std::size_t kInvalidId = std::numeric_limits<std::size_t>::max();



    struct MergeRule
    {
        TokenId left{kInvalidToken};
        TokenId right{kInvalidToken};
        TokenId newId{kInvalidToken};
    };

    explicit Bpe(const Vocab *vocab) noexcept;
    ~Bpe() override = default;

    Bpe(const Bpe &) = delete;
    Bpe &operator=(const Bpe &) = delete;
    Bpe(Bpe &&) = delete;
    Bpe &operator=(Bpe &&) = delete;

    [[nodiscard]] static UPtr createUniq(const Vocab *vocab)
    {
        return std::make_unique<Bpe>(vocab);
    }

    [[nodiscard]] static Ptr createShared(const Vocab *vocab)
    {
        return std::make_shared<Bpe>(vocab);
    }

    [[nodiscard]] TokenType type() const noexcept override
    {
        return TokenType::BPE;
    }

    void addMergeRule(TokenId left, TokenId right, TokenId newId);
    void setMergeRules(std::vector<MergeRule> rules);
    void clearRules() noexcept;

    std::size_t encode(std::string_view chunk, std::span<TokenId> outTokens) const override;
    [[nodiscard]] std::vector<TokenId> encode(std::span<const std::string> symbols) const override;
    std::size_t decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const override;
    // [[nodiscard]] virtual ByteSymbols decodeSymbols(std::span<const TokenId> tokens) const override;

    [[nodiscard]] std::span<const MergeRule> rules() const noexcept
    {
        return m_rules;
    }



private:
    struct Symbol
    {
        std::size_t prev{kInvalidId};
        std::size_t next{kInvalidId};
        TokenId id{kInvalidToken};
    };

    struct QueueElement
    {
        std::size_t pos{0};
        std::uint32_t rank{kInvalidRank};
        TokenId leftId{kInvalidToken};
        TokenId rightId{kInvalidToken};

        [[nodiscard]] bool operator>(const QueueElement &other) const noexcept
        {
            if (rank != other.rank)
                return rank > other.rank;

            return pos > other.pos;
        }
    };

    [[nodiscard]] static constexpr std::uint64_t makePairKey(TokenId left, TokenId right) noexcept
    {
        return
            (static_cast<std::uint64_t>(static_cast<std::uint32_t>(left)) << 32) |
               static_cast<std::uint64_t>(static_cast<std::uint32_t>(right));
    }

    [[nodiscard]] std::vector<TokenId> applyMerges(std::span<const TokenId> initialIds) const;

    void rebuildAccelerators();

    [[nodiscard]] TokenId findInitialByteToken(std::uint8_t byte) const noexcept;

private:
    std::vector<MergeRule> m_rules;
    std::unordered_map<std::uint64_t, std::uint32_t> m_pairToRank;
    std::array<std::uint32_t, 256 * 256> m_bytePairRank{};
};

} // namespace job::token