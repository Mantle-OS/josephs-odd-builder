#include "wordpiece.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "byte_fallback.h"

namespace job::token {

Wordpiece::Wordpiece(const Vocab *vocab, std::string prefix, std::size_t maxWordChars) :
    ITokenAlgo{vocab},
    m_prefix{std::move(prefix)},
    m_maxWordChars{maxWordChars}
{
    rebuildTries();
}

void Wordpiece::setPrefix(std::string prefix)
{
    m_prefix = std::move(prefix);
    rebuildTries();
}

void Wordpiece::rebuildTries()
{
    m_rootTrie.clear();
    m_continuationTrie.clear();

    if (!m_vocab)
        return;

    const std::string_view prefix = m_prefix;
    for (const auto &record : m_vocab->records()) {
        if (record.text().empty() || record.id() == kInvalidToken)
            continue;

        const std::string_view text = record.text();
        // Continuation token, e.g. "##ing".
        if (!prefix.empty() && text.starts_with(prefix)) {
            const std::string_view stripped = text.substr(prefix.size());
            if (!stripped.empty())
                m_continuationTrie.insert(stripped, record.id());

            continue;
        }

        m_rootTrie.insert(text, record.id());
    }
}

std::size_t Wordpiece::encode(std::string_view chunk, std::span<TokenId> outTokens) const
{
    if (chunk.empty() || outTokens.empty() || !m_vocab)
        return 0;

    const TokenId unkId = m_vocab->specialTokens().unkId();

    // UNRESOLVED:
    // chunk.size() counts UTF-8 bytes, not Unicode characters.
    // Decide later whether maxWordChars means bytes or codepoints.
    //
    // That word is way way way way way way way .... TOO LONG !!!
    if (chunk.size() > m_maxWordChars) {
        if (unkId == kInvalidToken)
            return 0;

        outTokens[0] = unkId;
        return 1;
    }

    std::size_t start = 0;
    std::size_t written = 0;

    while (start < chunk.size()) {
        // Buffer full: shit sticks :P
        if (written >= outTokens.size())
            return 0;

        const std::string_view suffix = chunk.substr(start);

        Trie::Match match{};
        if (start == 0)
            match = m_rootTrie.longestPrefix(suffix);
        else
            match = m_continuationTrie.longestPrefix(suffix);

        if (match.id == kInvalidToken || match.length == 0) {
            // WordPiece failure semantics:
            // emit one [UNK] for the entire input word.
            if (unkId == kInvalidToken)
                return 0;

            outTokens[0] = unkId;
            return 1;
        }

        outTokens[written++] = match.id;
        start += match.length;
    }

    return written;
}

std::size_t Wordpiece::decode(std::span<const TokenId> tokens, std::span<char> outBuffer) const
{
    if (tokens.empty() || outBuffer.empty() || !m_vocab)
        return 0;

    std::size_t written = 0;
    char *destination = outBuffer.data();

    const std::size_t capacity = outBuffer.size();
    const std::string_view prefix = m_prefix;

    for (const TokenId id : tokens) {
        if (id == kInvalidToken)
            continue;

        std::string_view piece = m_vocab->tokenText(id);

        if (piece.empty())
            continue;

        // UNRESOLVED:
        // Byte-fallback decoding policy probably belongs above
        // the WordPiece algorithm layer.
        std::uint8_t rawByte = 0;

        if (ByteFallback::parseByteToken(piece, rawByte)) {
            if (written >= capacity)
                break;

            destination[written++] = static_cast<char>(rawByte);

            continue;
        }

        // Continuation vocabulary stores pieces such as "##ing".
        // The marker is structural and is not emitted during decode.
        if (!prefix.empty() && piece.starts_with(prefix))
            piece = piece.substr(prefix.size());

        const std::size_t toCopy = std::min(piece.size(), capacity - written);
        std::memcpy(destination + written, piece.data(), toCopy);
        written += toCopy;
        if (written >= capacity)
            break;
    }

    return written;
}

} // namespace job::token