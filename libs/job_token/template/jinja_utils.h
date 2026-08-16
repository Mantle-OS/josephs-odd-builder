#pragma once

#include <string_view>

namespace job::token::utils {

[[nodiscard]] constexpr bool isWhitespace(char c) noexcept
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

[[nodiscard]] constexpr bool isIdentStart(char c) noexcept
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

[[nodiscard]] constexpr bool isIdentBody(char c) noexcept
{
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

[[nodiscard]] constexpr std::string_view trimTrailingWhitespace(std::string_view sv) noexcept {
    while (!sv.empty() && isWhitespace(sv.back())) {
        sv.remove_suffix(1);
    }
    return sv;
}

[[nodiscard]] constexpr std::string_view trimLeadingWhitespace(std::string_view sv) noexcept
{
    while (!sv.empty() && isWhitespace(sv.front())) {
        sv.remove_prefix(1);
    }
    return sv;
}

[[nodiscard]] constexpr std::string_view trim(std::string_view sv) noexcept
{
    return trimTrailingWhitespace(trimLeadingWhitespace(sv));
}

} // namespace job::token::utils
