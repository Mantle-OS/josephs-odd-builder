#pragma once

#include <cstdint>

namespace job::core::text {

enum class Utf16Status : std::uint8_t
{
    Complete = 0,
    OutputFull,
    Incomplete,
    Invalid
};

} // namespace job::core::text