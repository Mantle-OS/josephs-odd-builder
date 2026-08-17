#pragma once

#include <string_view>
#include <cstdint>

#include "jobtoken_export.h"
namespace job::token {


enum class GgufTokenType : uint8_t {
    Normal      = 1,
    Unknown     = 2,
    Control     = 3,
    UserDefined = 4,
    Unused      = 5,
    Byte        = 6
};

// should not be here should be in the enums file
enum class TokenType : uint8_t {
    Unknown = 0,
    BPE,
    WordPiece,
    Unigram,
    WordLevel
};
// should not be here should be in the enums file
[[nodiscard]] JOBTOKEN_EXPORT constexpr std::string_view tokenTypeString(TokenType type) noexcept
{
    switch (type) {
    case TokenType::BPE:
        return "BPE";
    case TokenType::WordPiece:
        return "WordPiece";
    case TokenType::Unigram:
        return "Unigram";
    case TokenType::WordLevel:
        return "WordLevel";
    case TokenType::Unknown:
    default: return "Unknown";
    }
}
// should not be here should be in the enums file
[[nodiscard]] JOBTOKEN_EXPORT constexpr TokenType tokenTypeFromStr(std::string_view str) noexcept
{
    if (str == "BPE")
        return TokenType::BPE;

    if (str == "WordPiece")
        return TokenType::WordPiece;

    if (str == "Unigram")
        return TokenType::Unigram;

    if (str == "WordLevel")
        return TokenType::WordLevel;

    return TokenType::Unknown;
}

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
    Middle,
    Cls,
    Sep,
};


// Structural token semantic flags
enum class StructuralType : uint8_t {
    Normal = 0,
    Unknown,
    Control,
    UserDefined,
    Unused,
    Byte
};



enum class TokenError : uint32_t {
    Success = 0,
    InvalidUtf8,
    BufferTooSmall,
    VocabEmpty,
    TokenNotFound,
    MergeRuleCorrupted,
    ContextInvalid,
    IoError,
    UnsupportedAlgorithm,
    OutOfMemory,
    UnknownError
};

[[nodiscard]] JOBTOKEN_EXPORT constexpr std::string_view errorString(TokenError code) noexcept
{
    switch (code) {
    case TokenError::Success:
        return "Success";
    case TokenError::InvalidUtf8:
        return "Input string contains invalid UTF-8 sequences";
    case TokenError::BufferTooSmall:
        return "Provided output span buffer is too small";
    case TokenError::VocabEmpty:
        return "Vocabulary is empty or uninitialized";
    case TokenError::TokenNotFound:
        return "Token string not found in vocabulary";
    case TokenError::MergeRuleCorrupted:
        return "BPE merge table entry is malformed";
    case TokenError::ContextInvalid:
        return "Tokenizer context state is invalid";
    case TokenError::IoError:
        return "Failed to read/write tokenizer binary payload";
    case TokenError::UnsupportedAlgorithm:
        return "Specified tokenizer algorithm is not supported";
    case TokenError::OutOfMemory:
        return "Memory allocation failed";
    case TokenError::UnknownError:
    default:
        return "Unknown tokenizer error";
    }
}



enum class CharType : uint8_t {
    Whitespace,
    Letter,
    Digit,
    Punctuation,
    Other
};

// ===================
enum class ChatRole : uint8_t {
    System = 0,
    User,
    Assistant,
    Tool,
    Custom
};

[[nodiscard]] JOBTOKEN_EXPORT constexpr std::string_view roleToString(ChatRole role) noexcept
{
    switch (role) {
    case ChatRole::System:
        return "system";
    case ChatRole::User:
        return "user";
    case ChatRole::Assistant:
        return "assistant";
    case ChatRole::Tool:
        return "tool";
    case ChatRole::Custom:
        return "custom";
    default:
        return "user";
    }
}

[[nodiscard]] JOBTOKEN_EXPORT constexpr ChatRole stringToRole(std::string_view str) noexcept
{
    if (str == "system")
        return ChatRole::System;

    if (str == "user")
        return ChatRole::User;

    if (str == "assistant")
        return ChatRole::Assistant;

    if (str == "tool")
        return ChatRole::Tool;
    return ChatRole::Custom;
}

// ===================


enum class SplitPattern : std::uint8_t {
    None = 0,

    GPT2,
    R50K,
    P50K,
    P50KEdit,
    CL100K,
    O200K,
    O200KHarmony,

    GPT4,
    LLaMA3,
    Qwen2,

    Custom
};


enum class ChatType : uint8_t {
    Raw = 0,
    ChatML,     // <|im_start|>role\ncontent<|im_end|>\n
    LLaMA3,     // <|start_header_id|>role<|end_header_id|>\n\ncontent<|eot_id|>
    Gemma,      // <start_of_turn>role\ncontent<end_of_turn>\n
    Mistral,    // [INST] prompt [/INST] response</s>
    Custom
};
// =======================================================

enum class ByteEncoding : std::uint8_t
{
    Raw = 0,
    Gpt2
};

} // namespace