#pragma once

#include <cassert>
#include <cstdint>

#include "view.h"

namespace job::ai::cords {

class Matrix : public ViewR {
public:
    using View  = ViewR;
    using Extent = View::Extent;
    using View::View;

    Matrix(core::real_t *data, std::uint32_t rows, std::uint32_t cols) :
        View(data, Extent{rows, cols})
    {
        assert(rank() == 2);
    }

    [[nodiscard]] core::real_t &at(std::uint32_t row, std::uint32_t col)
    {
        assert(rank() == 2);
        assert(row < m_extent[0] && col < m_extent[1]);
        return (*this)(row, col);
    }

    [[nodiscard]] const core::real_t &at(std::uint32_t row, std::uint32_t col) const
    {
        assert(rank() == 2);
        assert(row < m_extent[0] && col < m_extent[1]);
        return (*this)(row, col);
    }

    [[nodiscard]] std::uint32_t rows() const
    {
        return m_extent[0];
    }

    [[nodiscard]] std::uint32_t cols() const
    {
        return m_extent[1];
    }
};

} // namespace job::ai::cords
