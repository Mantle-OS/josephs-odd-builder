#pragma once

#include "job_lattice_coordinate.h"

namespace job::core::lattice {



struct alignas(16) LatticePoint
{
    LatticeCoordinate coordinate{};
};

static_assert(sizeof(LatticePoint) == 16, "LatticePoint must be 16 bytes");
static_assert(alignof(LatticePoint) == 16, "LatticePoint must be 16-byte aligned");

} // namespace job::core::lattice