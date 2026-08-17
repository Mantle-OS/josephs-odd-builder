#include "bpe.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <queue>
#include <vector>

#include "byte_fallback.h"

namespace job::token {

Bpe::Bpe(const Vocab *vocab) noexcept :
    ITokenAlgo{vocab}
{
    m_bytePairRank.fill(kInvalidRank);
}

void Bpe::addMergeRule(TokenId left, TokenId right, TokenId newId)
{
    const std::uint32_t rank = static_cast<std::uint32_t>(m_rules.size());
    m_rules.push_back({left, right, newId});
    const std::uint64_t key = makePairKey(left, right);
    m_pairToRank[key] = rank;

    if (left >= 0 && left < 256 && right >= 0 && right < 256)
        m_bytePairRank[ static_cast<std::size_t>(left * 256 + right) ] = rank;
}

void Bpe::setMergeRules(std::vector<MergeRule> rules)
{
    m_rules = std::move(rules);
    rebuildAccelerators();
}

void Bpe::clearRules() noexcept
{
    m_rules.clear();
    m_pairToRank.clear();
    m_bytePairRank.fill(kInvalidRank);
}

void Bpe::rebuildAccelerators()
{
    m_pairToRank.clear();
    m_pairToRank.reserve(m_rules.size() * 2);
    m_bytePairRank.fill(kInvalidRank);

    for (std::size_t index = 0; index < m_rules.size(); ++index) {
        const std::uint32_t rank = static_cast<std::uint32_t>(index);
        const MergeRule &rule = m_rules[index];
        const std::uint64_t key = makePairKey(rule.left, rule.right);

        m_pairToRank[key] = rank;
        if (rule.left >= 0 && rule.left < 256 && rule.right >= 0 && rule.right < 256)
            m_bytePairRank[ static_cast<std::size_t>( rule.left * 256 + rule.right) ] = rank;
    }
}

// UNRESOLVED:
// Initial byte-to-token resolution is tokenizer/provider policy, not
// fundamentally BPE policy. This currently resolves literal-byte and
// <0xNN> fallback tokens here until that responsibility is moved above
// the algorithm layer.
TokenId Bpe::findInitialByteToken(std::uint8_t byte) const noexcept
{
    if (!m_vocab)
        return kInvalidToken;

    const char character = static_cast<char>(byte);

    TokenId id = m_vocab->findId(
        std::string_view{&character, 1});

    if (id != kInvalidToken)
        return id;

    return m_vocab->findId(
        ByteFallback::byteToStringView(byte));
}


std::vector<TokenId> Bpe::applyMerges(std::span<const TokenId> initialIds) const
{
    if (initialIds.empty())
        return {};

    if (m_rules.empty() || m_pairToRank.empty()) {
        return std::vector<TokenId>{
            initialIds.begin(),
            initialIds.end()
        };
    }

    const std::size_t count = initialIds.size();
    std::vector<Symbol> symbols(count);
    for (std::size_t i = 0; i < count; ++i) {
        symbols[i].prev = i == 0 ? kInvalidId : i - 1;
        symbols[i].next = i + 1 == count ? kInvalidId : i + 1;
        symbols[i].id = initialIds[i];
    }

    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<>> queue;

    const auto pushPair = [&](std::size_t position) {
        if (position == kInvalidId || position >= symbols.size())
            return;

        const std::size_t next = symbols[position].next;
        if (next == kInvalidId || next >= symbols.size())
            return;

        const TokenId leftId = symbols[position].id;
        const TokenId rightId = symbols[next].id;
        const std::uint64_t key = makePairKey(leftId, rightId);

        const auto it = m_pairToRank.find(key);
        if (it == m_pairToRank.end())
            return;

        queue.push(QueueElement{ position, it->second, leftId, rightId });
    };

    for (std::size_t i = 0; i + 1 < count; ++i)
        pushPair(i);

    while (!queue.empty()) {
        const QueueElement top = queue.top();

        queue.pop();

        if (top.pos >= symbols.size())
            continue;

        if (symbols[top.pos].id != top.leftId)
            continue;

        const std::size_t next = symbols[top.pos].next;

        if (next == kInvalidId || next >= symbols.size() || symbols[next].id != top.rightId)
            continue;

        if (top.rank >= m_rules.size())
            continue;

        symbols[top.pos].id = m_rules[top.rank].newId;

        const std::size_t nextNext = symbols[next].next;

        symbols[top.pos].next = nextNext;

        if (nextNext != kInvalidId)
            symbols[nextNext].prev = top.pos;

        pushPair(symbols[top.pos].prev);
        pushPair(top.pos);
    }

    std::vector<TokenId> result;
    result.reserve(initialIds.size());

    std::size_t current = 0;

    while (current != kInvalidId && current < symbols.size()) {
        result.push_back(symbols[current].id);
        current = symbols[current].next;
    }

    return result;
}

std::size_t Bpe::encode(std::string_view chunk, std::span<TokenId> outTokens) const
{
    if (chunk.empty() || outTokens.empty())
        return 0;

    std::vector<TokenId> initialIds;
    initialIds.reserve(chunk.size());

    for (const char character : chunk) {
        initialIds.push_back(
            findInitialByteToken(
                static_cast<std::uint8_t>(character)));
    }

    const std::vector<TokenId> merged =
        applyMerges(initialIds);

    const std::size_t count =
        std::min(
            merged.size(),
            outTokens.size());

    std::copy_n(merged.begin(), count, outTokens.begin());
    return count;
}




std::vector<TokenId> Bpe::encode(std::span<const std::string> symbols) const
{
    if (symbols.empty() || !m_vocab)
        return {};

    std::vector<TokenId> initialIds;
    initialIds.reserve(symbols.size());

    for (const std::string &symbol : symbols)
        initialIds.push_back(m_vocab->findId(symbol));

    return applyMerges(initialIds);
}

std::size_t Bpe::decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const
{
    if (tokens.empty() || outBuffer.empty() || !m_vocab)
        return 0;

    std::size_t written = 0;
    char *destination = outBuffer.data();

    const std::size_t capacity =
        outBuffer.size();

    for (const TokenId id : tokens) {
        if (id == kInvalidToken)
            continue;

        const std::string_view piece = m_vocab->tokenText(id);

        if (piece.empty())
            continue;

        std::uint8_t rawByte = 0;
        if (ByteFallback::parseByteToken(piece, rawByte)) {
            if (written >= capacity)
                break;

            destination[written++] = static_cast<char>(rawByte);
            continue;
        }

        const std::size_t toCopy = std::min(piece.size(), capacity - written);
        std::memcpy(destination + written, piece.data(), toCopy);
        written += toCopy;
        if (written >= capacity)
            break;
    }

    return written;
}

} // namespace job::token