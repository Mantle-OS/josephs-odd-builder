#pragma once

#include <cstddef>

namespace job::core::lattice {

template <typename Status>
struct LatticeTransformResult
{
    std::size_t inputConsumed{};
    std::size_t outputProduced{};
    Status      status{};
};

} // namespace job::core::lattice