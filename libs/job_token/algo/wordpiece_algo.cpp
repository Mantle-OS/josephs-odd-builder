#include "algo/wordpiece_algo.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace job::token {

WordpieceAlgo::WordpieceAlgo(Vocab::Ptr vocab, std::string continuationPrefix, size_t maxInputCharsPerWord) :
    ITokenAlgo{std::move(vocab)},
    m_continuationPrefix{std::move(continuationPrefix)},
    m_maxInputCharsPerWord{maxInputCharsPerWord}
{
    rebuildTries();
}

void WordpieceAlgo::setContinuationPrefix(std::string prefix)
{
    m_continuationPrefix = std::move(prefix);
    rebuildTries();
}

void WordpieceAlgo::rebuildTries()
{
    m_rootTrie.clear();
    m_continuationTrie.clear();

    if (!m_vocab)
        return;

    const auto records = m_vocab->records();
    const std::string_view prefixView = m_continuationPrefix;

    for (const auto& record : records) {
        if (record.text.empty() || record.id == kInvalidToken)
            continue;

        const std::string_view text = record.text;

        // Check if token represents a continuation subword (e.g. "##ing")
        if (!prefixView.empty() && text.starts_with(prefixView)) {
            const std::string_view stripped = text.substr(prefixView.size());
            if (!stripped.empty())
                m_continuationTrie.insert(stripped, record.id);
        } else {
            m_rootTrie.insert(text, record.id);
        }
    }
}

size_t WordpieceAlgo::encodeChunk(std::string_view chunk, std::span<TokenId> outTokens) const
{
    if (chunk.empty() || outTokens.empty() || !m_vocab)
        return 0;

    const TokenId unkId = m_vocab->specialTokens().unkId();

    // That word is way way way way way way way .... TO LONG !!!
    if (chunk.size() > m_maxInputCharsPerWord) {
        if (unkId != kInvalidToken) {
            outTokens[0] = unkId;
            return 1;
        }
        return 0;
    }

    size_t start = 0;
    size_t outCount = 0;
    const size_t maxTokens = outTokens.size();

    while (start < chunk.size()) {
        // Buffer full: Shit sticks :P
        if (outCount >= maxTokens)
            return 0;

        const std::string_view suffix = chunk.substr(start);
        Trie::Match match{};

        if (start == 0)
            match = m_rootTrie.longestPrefix(suffix); // First subword: match root vocabulary
        else
            match = m_continuationTrie.longestPrefix(suffix); // Continuation subword: match stripped continuation vocabulary

        if (match.id == kInvalidToken || match.length == 0) {
            // Word cannot be completely segmented by WordPiece:
            // Fall back to emitting single [UNK] token for the entire chunk
            if (unkId != kInvalidToken && maxTokens > 0) {
                outTokens[0] = unkId;
                return 1;
            }
            return 0;
        }

        outTokens[outCount++] = match.id;
        start += match.length;
    }

    return outCount;
}

size_t WordpieceAlgo::decodeTokens(std::span<const TokenId> tokens, std::span<char> outBuffer) const
{
    if (tokens.empty() || outBuffer.empty() || !m_vocab)
        return 0;

    size_t written = 0;
    char* dest = outBuffer.data();
    const size_t maxCap = outBuffer.size();
    const std::string_view prefixView = m_continuationPrefix;

    for (const TokenId id : tokens) {
        if (id == kInvalidToken)
            continue;

        std::string_view piece = m_vocab->tokenText(id);
        if (piece.empty())
            continue;

        // Handle hex byte fallback: "<0xNN>"
        uint8_t rawByte = 0;
        if (ByteFallback::parseByteToken(piece, rawByte)) {
            if (written + 1 > maxCap)
                break;
            dest[written++] = static_cast<char>(rawByte);
            continue;
        }

        // Strip continuation prefix ("##") when decoding
        if (!prefixView.empty() && piece.starts_with(prefixView))
            piece = piece.substr(prefixView.size());

        const size_t toCopy = std::min(piece.size(), maxCap - written);
        std::memcpy(dest + written, piece.data(), toCopy);
        written += toCopy;

        if (written >= maxCap)
            break;
    }

    return written;
}

} // namespace job::token