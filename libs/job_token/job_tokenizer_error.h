#pragma once

#include <cstdint>
#include <string_view>
#include "jobtoken_export.h"
namespace job::token {

enum class TokenizerErrorCode : uint32_t {
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

[[nodiscard]] JOBTOKEN_EXPORT constexpr std::string_view errorToString(TokenizerErrorCode code) noexcept
{
    switch (code) {
    case TokenizerErrorCode::Success:
        return "Success";
    case TokenizerErrorCode::InvalidUtf8:
        return "Input string contains invalid UTF-8 sequences";
    case TokenizerErrorCode::BufferTooSmall:
        return "Provided output span buffer is too small";
    case TokenizerErrorCode::VocabEmpty:
        return "Vocabulary is empty or uninitialized";
    case TokenizerErrorCode::TokenNotFound:
        return "Token string not found in vocabulary";
    case TokenizerErrorCode::MergeRuleCorrupted:
        return "BPE merge table entry is malformed";
    case TokenizerErrorCode::ContextInvalid:
        return "Tokenizer context state is invalid";
    case TokenizerErrorCode::IoError:
        return "Failed to read/write tokenizer binary payload";
    case TokenizerErrorCode::UnsupportedAlgorithm:
        return "Specified tokenizer algorithm is not supported";
    case TokenizerErrorCode::OutOfMemory:
        return "Memory allocation failed";
    case TokenizerErrorCode::UnknownError:
    default:
        return "Unknown tokenizer error";
    }
}

} // namespace job::token