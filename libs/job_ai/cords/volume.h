#pragma once

#include <cassert>
#include <cstdint>

#include "view.h"

namespace job::ai::cords {

class Volume : public ViewR {
public:
    using View   = ViewR;
    using Extent = View::Extent;

    Volume(float *data,
           std::uint32_t dim0,
           std::uint32_t dim1,
           std::uint32_t dim2) :
        View(data, Extent{dim0, dim1, dim2})
    {
        assert(rank() == 3);
    }

    Volume(float *data, const Extent &extent) :
        View(data, extent)
    {
        assert(rank() == 3);
    }

    [[nodiscard]] float &at(std::uint32_t dim0,
                            std::uint32_t dim1,
                            std::uint32_t dim2)
    {
        assert(rank() == 3);
        assert(dim0 < m_extent[0] &&
               dim1 < m_extent[1] &&
               dim2 < m_extent[2]);
        return (*this)(dim0, dim1, dim2);
    }

    [[nodiscard]] const float &at(std::uint32_t dim0,
                                  std::uint32_t dim1,
                                  std::uint32_t dim2) const
    {
        assert(rank() == 3);
        assert(dim0 < m_extent[0] &&
               dim1 < m_extent[1] &&
               dim2 < m_extent[2]);
        return (*this)(dim0, dim1, dim2);
    }

    [[nodiscard]] std::uint32_t dim0() const
    {
        assert(rank() == 3);
        return m_extent[0];
    }

    [[nodiscard]] std::uint32_t dim1() const
    {
        assert(rank() == 3);
        return m_extent[1];
    }

    [[nodiscard]] std::uint32_t dim2() const
    {
        assert(rank() == 3);
        return m_extent[2];
    }
};

} // namespace job::ai::cords
