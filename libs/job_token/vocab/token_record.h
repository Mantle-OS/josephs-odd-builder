#pragma once

#include <string>

#include "job_tokenizer_types.h"
#include "jobtoken_export.h"

namespace job::token {

struct JOBTOKEN_EXPORT TokenRecord {
    std::string text;
    TokenId     id{kInvalidToken};
    float       score{0.0f};
    TokenType   type{TokenType::Normal};

    [[nodiscard]] constexpr bool isValid() const noexcept
    {
        return id != kInvalidToken;
    }

    [[nodiscard]] constexpr bool isSpecial() const noexcept
    {
        return type == TokenType::Control || type == TokenType::UserDefined;
    }

    [[nodiscard]] constexpr bool isByte() const noexcept
    {
        return type == TokenType::Byte;
    }

    [[nodiscard]] constexpr bool isUnused() const noexcept
    {
        return type == TokenType::Unused;
    }
};

} // namespace job::token