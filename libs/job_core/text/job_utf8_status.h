#pragma once

#include <cstdint>

namespace job::core::text {

enum class Utf8Status : std::uint8_t
{
    Complete,
    OutputFull,
    Incomplete,
    Invalid
};

} // namespace job::core::text