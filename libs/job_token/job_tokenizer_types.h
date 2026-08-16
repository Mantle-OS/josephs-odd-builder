#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace job::token {

// Standard discrete token identifier
using TokenId = int32_t;
inline constexpr TokenId kInvalidToken = -1;

// Maximum 21-bit coordinate space representation (7 bits per axis: X, Y, Z)
inline constexpr uint32_t kMax21BitTokenId = (1u << 21) - 1u;

// Tokenization algorithm category
enum class TokenizerAlgorithm : uint8_t {
    BPE = 0,
    Unigram,
    WordPiece,
    Motif,
    Custom
};

// Structural token semantic flags
enum class TokenType : uint8_t {
    Normal = 0,
    Unknown,
    Control,
    UserDefined,
    Unused,
    Byte
};

// Common special token classification
enum class SpecialTokenType : uint8_t {
    None = 0,
    Bos,
    Eos,
    Eot,
    Pad,
    Unk,
    Mask,
    Prefix,
    Suffix,
    Middle
};

// Token encoding options
struct TokenizeOptions {
    bool addBos          = true;
    bool addEos          = false;
    bool parseSpecial    = true;
    float defaultMass    = 1.0f;
};

// Character/byte offset slice
struct TokenSpan {
    size_t byteBegin = 0;
    size_t byteEnd   = 0;

    [[nodiscard]] constexpr size_t length() const noexcept
    {
        return (byteEnd >= byteBegin) ? (byteEnd - byteBegin) : 0;
    }
};

// Detailed encoding result
struct TokenizeResult {
    std::vector<TokenId>   tokens;
    std::vector<TokenSpan> spans;
};

} // namespace job::token