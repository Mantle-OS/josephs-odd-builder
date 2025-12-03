#pragma once

#include <cassert>
#include <cstdint>

#include "tensor_view.h"

namespace job::ai::base {

class Tensor3D : public TensorViewR {
public:
    using Base  = TensorViewR;
    using Shape = Base::Shape;
    using Base::Base;
    Tensor3D(core::real_t *data,
             std::uint32_t d0,
             std::uint32_t d1,
             std::uint32_t d2):
        Base(data, Shape{d0, d1, d2})
    {
        assert(rank() == 3);
    }

    [[nodiscard]] core::real_t &at(std::uint32_t d0,
                                   std::uint32_t d1,
                                   std::uint32_t d2)
    {
        assert(rank() == 3);
        assert(d0 < m_shape[0] && d1 < m_shape[1] && d2 < m_shape[2]);
        return (*this)(d0, d1, d2); // use generic indexing
    }

    [[nodiscard]] const core::real_t &at(std::uint32_t d0,
                                         std::uint32_t d1,
                                         std::uint32_t d2) const
    {
        assert(rank() == 3);
        assert(d0 < m_shape[0] && d1 < m_shape[1] && d2 < m_shape[2]);
        return (*this)(d0, d1, d2);
    }

    [[nodiscard]] std::uint32_t dim0() const
    {
        return m_shape[0];
    }
    [[nodiscard]] std::uint32_t dim1() const
    {
        return m_shape[1];
    }
    [[nodiscard]] std::uint32_t dim2() const
    {
        return m_shape[2];
    }
};

} // namespace job::ai::base
