#pragma once
#include <string_view>

// Originally generated from the RFC spec -> abnf ->  grammar using job_json_gen.py,
// then manually reduced and updated for job_json.
// Source history:
// https://github.com/Mantle-OS/job_json_gen


namespace job::json::grammar {

// Single code points
inline constexpr char32_t decimal_point = U'.';
inline constexpr char32_t minus = U'-';
inline constexpr char32_t plus = U'+';
inline constexpr char32_t zero = U'0';
inline constexpr char32_t escape = U'\\';
inline constexpr char32_t quotation_mark = U'"';

// Code-point ranges

constexpr bool is_digit1_9(char32_t c) noexcept
{
    return c >= 0x31 && c <= 0x39;
}

constexpr bool is_DIGIT(char32_t c) noexcept
{
    return c >= 0x30 && c <= 0x39;
}

// Character predicates

constexpr bool is_e(char32_t c) noexcept
{
    return c == 0x65 ||
           c == 0x45;
}

constexpr bool is_unescaped(char32_t c) noexcept
{
    return (c >= 0x20 && c <= 0x21) ||
           (c >= 0x23 && c <= 0x5B) ||
           (c >= 0x5D && c <= 0x10FFFF);
}

constexpr bool is_HEXDIG(char32_t c) noexcept
{
    return is_DIGIT(c) ||
           c == U'a' || c == U'A' ||
           c == U'b' || c == U'B' ||
           c == U'c' || c == U'C' ||
           c == U'd' || c == U'D' ||
           c == U'e' || c == U'E' ||
           c == U'f' || c == U'F';
}

// Literal strings

inline constexpr std::string_view false_ = "false";
inline constexpr std::string_view null = "null";
inline constexpr std::string_view true_ = "true";

} // namespace job::json::grammar
