#pragma once

#include <cstdint>

namespace job::core::text {

enum class TextEncoding : std::uint8_t
{
    Utf8,
    Utf16,
    Utf32
};

} // namespace job::core::text