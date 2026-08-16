#include "algo/unigram_algo.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace job::token {

UnigramAlgo::UnigramAlgo(Vocab::Ptr vocab) :
    ITokenAlgo{std::move(vocab)}
{
    rebuildTrie();
}

void UnigramAlgo::rebuildTrie()
{
    m_trie.clear();
    if (!m_vocab)
        return;

    const auto records = m_vocab->records();
    for (const auto& record : records) {
        if (!record.text.empty() && record.id != kInvalidToken) {
            m_trie.insert(record.text, record.id);
        }
    }

    const TokenId unkId = m_vocab->specialTokens().unkId();
    if (unkId != kInvalidToken)
        m_unkScore = m_vocab->tokenScore(unkId);
    else
        m_unkScore = -10.0f;
}

TokenId UnigramAlgo::findByteFallbackToken(uint8_t byteVal) const noexcept
{
    if (!m_vocab)
        return kInvalidToken;

    // ASCII
    const char singleChar[2] = {static_cast<char>(byteVal), '\0'};
    TokenId id = m_vocab->findId(std::string_view(singleChar, 1));
    if (id != kInvalidToken)
        return id;

    // "<0xNN>"
    const std::string_view hexView = ByteFallback::byteToStringView(byteVal);
    id = m_vocab->findId(hexView);
    if (id != kInvalidToken)
        return id;

    return kInvalidToken;
}

size_t UnigramAlgo::encodeChunk(std::string_view chunk, std::span<TokenId> outTokens) const
{
    if (chunk.empty() || outTokens.empty() || !m_vocab)
        return 0;

    const size_t n = chunk.size();
    std::vector<DpNode> dp(n + 1);
    dp[0].bestScore = 0.0f;

    std::vector<Trie::Match> matches;
    matches.reserve(32);

    const TokenId unkId = m_vocab->specialTokens().unkId();

    // Forward Viterbi Search
    for (size_t i = 0; i < n; ++i) {
        // Skip unreachable states (Fast-Math safe check)
        if (dp[i].bestScore <= kMinValidScore)
            continue;

        const std::string_view suffix = chunk.substr(i);
        matches.clear();
        m_trie.findAllPrefixes(suffix, matches);

        bool matchedAny = false;

        for (const auto& match : matches) {
            const float score = m_vocab->tokenScore(match.id);
            const float candidateScore = dp[i].bestScore + score;
            const size_t targetIdx = i + match.length;

            if (candidateScore > dp[targetIdx].bestScore) {
                dp[targetIdx].bestScore = candidateScore;
                dp[targetIdx].bestToken = match.id;
                dp[targetIdx].bestPrev  = static_cast<uint32_t>(i);
                matchedAny = true;
            }
        }

        // Single-byte fallback if no prefix matched, or to guarantee an unbroken lattice
        const uint8_t rawByte = static_cast<uint8_t>(chunk[i]);
        const TokenId byteToken = findByteFallbackToken(rawByte);

        if (byteToken != kInvalidToken) {
            const float byteScore = m_vocab->tokenScore(byteToken);
            const float candidateScore = dp[i].bestScore + byteScore;
            if (candidateScore > dp[i + 1].bestScore) {
                dp[i + 1].bestScore = candidateScore;
                dp[i + 1].bestToken = byteToken;
                dp[i + 1].bestPrev  = static_cast<uint32_t>(i);
            }
        } else if (!matchedAny && unkId != kInvalidToken) {
            const float candidateScore = dp[i].bestScore + m_unkScore - 5.0f; // UNK penalty
            if (candidateScore > dp[i + 1].bestScore) {
                dp[i + 1].bestScore = candidateScore;
                dp[i + 1].bestToken = unkId;
                dp[i + 1].bestPrev  = static_cast<uint32_t>(i);
            }
        }
    }

    // If no path reached the end, return 0 (Fast-Math safe check)
    if (dp[n].bestToken == kInvalidToken || dp[n].bestScore <= kMinValidScore)
        return 0;

    // Backtrack optimal path
    std::vector<TokenId> path;
    path.reserve(n);

    size_t curr = n;
    while (curr > 0) {
        path.push_back(dp[curr].bestToken);
        curr = dp[curr].bestPrev;
    }

    std::reverse(path.begin(), path.end());

    const size_t outCount = std::min(path.size(), outTokens.size());
    std::copy_n(path.begin(), outCount, outTokens.begin());
    return outCount;
}

size_t UnigramAlgo::decodeTokens(std::span<const TokenId> tokens, std::span<char> outBuffer) const
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

        // Handle hex byte fallback token: "<0xNN>"
        uint8_t rawByte = 0;
        if (ByteFallback::parseByteToken(piece, rawByte)) {
            if (written + 1 > maxCap)
                break;
            dest[written++] = static_cast<char>(rawByte);
            continue;
        }

        // Copy plain text subword piece
        const size_t toCopy = std::min(piece.size(), maxCap - written);
        std::memcpy(dest + written, piece.data(), toCopy);
        written += toCopy;

        if (written >= maxCap)
            break;
    }

    return written;
}

} // namespace job::token