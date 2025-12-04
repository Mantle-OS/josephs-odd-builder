#pragma once

#include <cassert>
#include <cstdint>

#include "view.h"

namespace job::ai::cords {

class Fiber : public ViewR {
public:
    using View = ViewR;
    using Extent = View::Extent;
    using View::View;

    Fiber(core::real_t *data, std::uint32_t size1) :
        View(data, Extent{size1})
    {
        assert(rank() == 1);
    }

    [[nodiscard]] core::real_t &at(std::uint32_t size1)
    {
        assert(rank() == 1);
        assert(size1 < m_extent[0]);
        return (*this)(size1);
    }

    [[nodiscard]] const core::real_t &at(std::uint32_t size1) const
    {
        assert(rank() == 1);
        assert(size1 < m_extent[0]);
        return (*this)(size1);
    }

    [[nodiscard]] std::uint32_t size1() const
    {
        return m_extent[0];
    }
};

} // namespace job::ai::cords
