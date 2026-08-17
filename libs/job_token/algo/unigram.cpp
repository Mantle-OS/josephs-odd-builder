#include "unigram.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "byte_fallback.h"

namespace job::token {

Unigram::Unigram(const Vocab *vocab) noexcept :
    ITokenAlgo{vocab}
{
    rebuildTrie();
}

void Unigram::rebuildTrie()
{
    m_trie.clear();
    if (!m_vocab)
        return;

    for (const auto &record : m_vocab->records())
        if (!record.text().empty() && record.id() != kInvalidToken)
            m_trie.insert(record.text(), record.id());

    const TokenId unkId =
        m_vocab->specialTokens().unkId();

    if (unkId != kInvalidToken)
        m_unkScore = m_vocab->tokenScore(unkId);
    else
        m_unkScore = -10.0f;
}

// UNRESOLVED:
// Initial byte/token fallback policy probably belongs above the
// Unigram algorithm layer. Keep this here temporarily while the
// ownership and tokenizer-policy boundaries are being sorted.
TokenId Unigram::findByteFallbackToken(std::uint8_t byte) const noexcept
{
    if (!m_vocab)
        return kInvalidToken;

    const char character = static_cast<char>(byte);

    TokenId id = m_vocab->findId(std::string_view{&character, 1});
    if (id != kInvalidToken)
        return id;

    id = m_vocab->findId(ByteFallback::byteToStringView(byte));

    return id;
}

std::size_t Unigram::encode(std::string_view chunk, std::span<TokenId> outTokens) const
{
    if (chunk.empty() || outTokens.empty() || !m_vocab)
        return 0;

    const std::size_t count = chunk.size();
    std::vector<DpNode> dp(count + 1);
    dp[0].bestScore = 0.0f;

    std::vector<Trie::Match> matches;
    matches.reserve(32);

    const TokenId unkId = m_vocab->specialTokens().unkId();
    // Forward Viterbi search.
    for (std::size_t i = 0; i < count; ++i) {
        // Fast-math-safe unreachable-state check.
        if (dp[i].bestScore <= kMinValidScore)
            continue;

        const std::string_view suffix = chunk.substr(i);

        matches.clear();
        m_trie.findAllPrefixes(suffix, matches);
        bool matchedAny = false;

        for (const Trie::Match &match : matches) {
            const std::size_t target = i + match.length;
            if (target > count)
                continue;

            const float score = m_vocab->tokenScore(match.id);
            const float candidateScore = dp[i].bestScore + score;

            if (candidateScore > dp[target].bestScore) {
                dp[target].bestScore = candidateScore;
                dp[target].bestToken = match.id;
                dp[target].bestPrev = i;
                matchedAny = true;
            }
        }

        // UNRESOLVED:
        // Byte fallback is currently used here to keep the lattice
        // connected, but that policy likely belongs above Unigram.
        const std::uint8_t rawByte = static_cast<std::uint8_t>(chunk[i]);
        const TokenId byteToken = findByteFallbackToken(rawByte);

        if (byteToken != kInvalidToken) {
            const float byteScore = m_vocab->tokenScore(byteToken);
            const float candidateScore = dp[i].bestScore + byteScore;
            if (candidateScore > dp[i + 1].bestScore) {
                dp[i + 1].bestScore = candidateScore;
                dp[i + 1].bestToken = byteToken;
                dp[i + 1].bestPrev = i;
            }
        } else if (!matchedAny && unkId != kInvalidToken) {
            const float candidateScore = dp[i].bestScore + m_unkScore - 5.0f;
            if (candidateScore > dp[i + 1].bestScore) {

                dp[i + 1].bestScore = candidateScore;
                dp[i + 1].bestToken = unkId;
                dp[i + 1].bestPrev = i;
            }
        }
    }

    // No valid path reached the end.
    if (dp[count].bestToken == kInvalidToken || dp[count].bestScore <= kMinValidScore)
        return 0;

    std::vector<TokenId> path;
    path.reserve(count);
    std::size_t current = count;
    while (current > 0) {
        const DpNode &node = dp[current];

        if (node.bestToken == kInvalidToken)
            return 0;

        if (node.bestPrev >= current)
            return 0;

        path.push_back(node.bestToken);

        current = node.bestPrev;
    }

    std::reverse(path.begin(), path.end());
    const std::size_t outCount = std::min(path.size(), outTokens.size());
    std::copy_n(path.begin(), outCount, outTokens.begin());

    return outCount;
}

std::size_t Unigram::decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const
{
    if (tokens.empty() || outBuffer.empty() || !m_vocab)
        return 0;

    std::size_t written = 0;
    char *destination = outBuffer.data();
    const std::size_t capacity = outBuffer.size();

    for (const TokenId id : tokens) {
        if (id == kInvalidToken)
            continue;

        const std::string_view piece = m_vocab->tokenText(id);

        if (piece.empty())
            continue;

        // UNRESOLVED:
        // Byte-fallback decoding policy may also belong above
        // the Unigram algorithm layer.
        std::uint8_t rawByte = 0;
        if (ByteFallback::parseByteToken( piece, rawByte)) {
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