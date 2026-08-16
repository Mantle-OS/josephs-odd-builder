#include "algo/bpe_algo.h"

#include <queue>
#include <cstring>
#include <algorithm>

namespace job::token {

BpeAlgo::BpeAlgo(Vocab::Ptr vocab) :
    ITokenAlgo{std::move(vocab)}
{
    m_bytePairRank.fill(kInvalidRank);
}

void BpeAlgo::addMergeRule(TokenId left, TokenId right, TokenId newId)
{
    const uint32_t rank = static_cast<uint32_t>(m_rules.size());
    m_rules.push_back({left, right, newId});

    const uint64_t key = makePairKey(left, right);
    m_pairToRank[key] = rank;

    if (left >= 0 && left < 256 && right >= 0 && right < 256)
        m_bytePairRank[static_cast<size_t>(left * 256 + right)] = rank;
}

void BpeAlgo::setMergeRules(std::vector<MergeRule> rules)
{
    m_rules = std::move(rules);
    rebuildAccelerators();
}

void BpeAlgo::clearRules() noexcept
{
    m_rules.clear();
    m_pairToRank.clear();
    m_bytePairRank.fill(kInvalidRank);
}

void BpeAlgo::rebuildAccelerators()
{
    m_pairToRank.clear();
    m_pairToRank.reserve(m_rules.size() * 2);
    m_bytePairRank.fill(kInvalidRank);

    for (uint32_t rank = 0; rank < m_rules.size(); ++rank) {
        const auto& r = m_rules[rank];
        const uint64_t key = makePairKey(r.left, r.right);
        m_pairToRank[key] = rank;

        if (r.left >= 0 && r.left < 256 && r.right >= 0 && r.right < 256)
            m_bytePairRank[static_cast<size_t>(r.left * 256 + r.right)] = rank;
    }
}

TokenId BpeAlgo::findInitialByteToken(uint8_t byteVal) const noexcept
{
    if (!m_vocab) {
        return static_cast<TokenId>(byteVal);
    }

    // single character exists in vocab
    const char singleChar[2] = {static_cast<char>(byteVal), '\0'};
    TokenId id = m_vocab->findId(std::string_view(singleChar, 1));
    if (id != kInvalidToken)
        return id;

    // <0xNN>
    const std::string_view hexView = ByteFallback::byteToStringView(byteVal);
    id = m_vocab->findId(hexView);
    if (id != kInvalidToken)
        return id;

    // (fallback) direct raw numeric ID
    return static_cast<TokenId>(byteVal);
}

size_t BpeAlgo::encodeChunk(std::string_view chunk, std::span<TokenId> outTokens) const
{
    if (chunk.empty() || outTokens.empty())
        return 0;

    // Fast path: if no merge rules exist, map directly to byte tokens
    if (m_rules.empty() || m_pairToRank.empty()) {
        const size_t count = std::min(chunk.size(), outTokens.size());
        for (size_t i = 0; i < count; ++i) {
            outTokens[i] = findInitialByteToken(static_cast<uint8_t>(chunk[i]));
        }
        return count;
    }

    const uint32_t n = static_cast<uint32_t>(chunk.size());
    std::vector<Symbol> syms(n);

    for (uint32_t i = 0; i < n; ++i) {
        syms[i].prev = (i == 0) ? kInvalidId : (i - 1);
        syms[i].next = (i + 1 == n) ? kInvalidId : (i + 1);
        syms[i].id   = findInitialByteToken(static_cast<uint8_t>(chunk[i]));
    }

    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<>> pq;

    auto pushPair = [&](uint32_t pos) {
        if (pos == kInvalidId)
            return;

        const uint32_t next = syms[pos].next;
        if (next == kInvalidId)
            return;

        const TokenId leftId  = syms[pos].id;
        const TokenId rightId = syms[next].id;

        uint32_t rank = kInvalidRank;

        if (leftId >= 0 && leftId < 256 && rightId >= 0 && rightId < 256) {
            rank = m_bytePairRank[static_cast<size_t>(leftId * 256 + rightId)];
        } else {
            const uint64_t key = makePairKey(leftId, rightId);
            auto it = m_pairToRank.find(key);
            if (it != m_pairToRank.end())
                rank = it->second;
        }

        if (rank != kInvalidRank)
            pq.push(QueueElement{pos, rank, leftId, rightId});
    };

    for (uint32_t i = 0; i + 1 < n; ++i)
        pushPair(i);

    while (!pq.empty()) {
        const auto top = pq.top();
        pq.pop();

        // Stale queue element validations
        if (syms[top.pos].id != top.leftId)
            continue;

        const uint32_t next = syms[top.pos].next;
        if (next == kInvalidId || syms[next].id != top.rightId)
            continue;

        if (top.rank >= m_rules.size())
            continue;

        // Apply merge: update left symbol to newId, splice out right symbol
        const TokenId newId = m_rules[top.rank].newId;
        syms[top.pos].id = newId;

        const uint32_t nextNext = syms[next].next;
        syms[top.pos].next = nextNext;
        if (nextNext != kInvalidId)
            syms[nextNext].prev = top.pos;

        // Push newly formed adjacent pairs
        pushPair(syms[top.pos].prev);
        pushPair(top.pos);
    }

    // Drain active symbols into output span
    size_t outCount = 0;
    uint32_t cur = 0;
    while (cur != kInvalidId && outCount < outTokens.size()) {
        outTokens[outCount++] = syms[cur].id;
        cur = syms[cur].next;
    }

    return outCount;
}

size_t BpeAlgo::decodeTokens(std::span<const TokenId> tokens, std::span<char> outBuffer) const
{
    if (tokens.empty() || outBuffer.empty() || !m_vocab)
        return 0;

    size_t written = 0;
    char* dest = outBuffer.data();
    const size_t maxCap = outBuffer.size();

    for (const TokenId id : tokens) {
        if (id == kInvalidToken)
            continue;

        const std::string_view piece = m_vocab->tokenText(id);
        if (piece.empty())
            continue;

        // Check if token represents a fallback byte: "<0xNN>"
        uint8_t rawByte = 0;
        if (ByteFallback::parseByteToken(piece, rawByte)) {
            if (written + 1 > maxCap)
                break;

            dest[written++] = static_cast<char>(rawByte);
            continue;
        }

        // Otherwise copy string piece directly
        const size_t toCopy = std::min(piece.size(), maxCap - written);
        std::memcpy(dest + written, piece.data(), toCopy);
        written += toCopy;

        if (written >= maxCap)
            break;
    }

    return written;
}

} // namespace job::token