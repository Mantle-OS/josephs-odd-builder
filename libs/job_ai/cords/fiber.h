#pragma once

#include <cassert>
#include <cstdint>

#include "view.h"

namespace job::ai::cords {

class Fiber : public ViewR {
public:
    using View   = ViewR;
    using Extent = View::Extent;

    // Explicit 1D ctor
    Fiber(float *data, std::uint32_t size1):
        View(data, Extent{size1})
    {
        assert(rank() == 1);
    }

    // Optionally, a ctor from Extent that enforces rank==1
    Fiber(float *data, const Extent& extent):
        View(data, extent)
    {
        assert(rank() == 1);
    }

    [[nodiscard]] float &at(std::uint32_t i)
    {
        assert(rank() == 1);
        assert(i < m_extent[0]);
        return (*this)(i);
    }

    [[nodiscard]] const float &at(std::uint32_t i) const
    {
        assert(rank() == 1);
        assert(i < m_extent[0]);
        return (*this)(i);
    }

    [[nodiscard]] std::uint32_t size1() const
    {
        assert(rank() == 1);
        return m_extent[0];
    }
};

} // namespace job::ai::cords
