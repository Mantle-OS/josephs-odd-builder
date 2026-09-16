#pragma once

#include <cstdint>

namespace job::core::lattice {

enum class LatticeComponent : std::uint8_t
{
    X = 1,
    Y = 2,
    Z = 3,
    M = 4
};

struct alignas(16) LatticeCoordinate
{
    float x{};
    float y{};
    float z{};
    float m{};
};

static_assert(sizeof(LatticeCoordinate) == 16, "LatticeCoordinate must be 16 bytes");
static_assert(alignof(LatticeCoordinate) == 16, "LatticeCoordinate must be 16-byte aligned");

} // namespace job::core::lattice