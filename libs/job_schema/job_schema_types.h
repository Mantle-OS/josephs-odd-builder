#pragma once

#include <array>
#include <cstdint>

namespace job::schema {

using bin16 = std::array<std::uint8_t, 16>;
using bin32 = std::array<std::uint8_t, 32>;
using bin64 = std::array<std::uint8_t, 64>;
enum class SchemaFormat : std::uint8_t
{
    Json = 0,
    Yaml,
    Binary
};
} // namespace job::schema