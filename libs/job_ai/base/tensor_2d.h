#pragma once

#include <cassert>
#include <cstdint>

#include "tensor_view.h"

namespace job::ai::base {

class Tensor2D : public TensorViewR {
public:
    using Base  = TensorViewR;
    using Shape = Base::Shape;
    using Base::Base; // inherit TensorViewR constructors

    Tensor2D(core::real_t *data, std::uint32_t rows, std::uint32_t cols) :
        Base(data, Shape{rows, cols})
    {
        assert(rank() == 2);
    }

    [[nodiscard]] core::real_t &at(std::uint32_t row, std::uint32_t col)
    {
        assert(rank() == 2);
        assert(row < m_shape[0] && col < m_shape[1]);
        return (*this)(row, col); // generic indexing
    }

    [[nodiscard]] const core::real_t &at(std::uint32_t row, std::uint32_t col) const
    {
        assert(rank() == 2);
        assert(row < m_shape[0] && col < m_shape[1]);
        return (*this)(row, col);
    }

    [[nodiscard]] std::uint32_t rows() const
    {
        return m_shape[0];
    }

    [[nodiscard]] std::uint32_t cols() const
    {
        return m_shape[1];
    }
};

} // namespace job::ai::base
