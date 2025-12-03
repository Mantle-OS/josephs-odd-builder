#pragma once

#include <cassert>
#include <cstdint>

#include "tensor_view.h"

namespace job::ai::base {

class Tensor1D : public TensorViewR {
public:
    using Base  = TensorViewR;
    using Shape = Base::Shape;
    using Base::Base;

    Tensor1D(core::real_t *data, std::uint32_t size) :
        Base(data, Shape{size})
    {
        assert(rank() == 1);
    }

    [[nodiscard]] core::real_t &at(std::uint32_t i)
    {
        assert(rank() == 1);
        assert(i < m_shape[0]);
        return (*this)(i);
    }

    [[nodiscard]] const core::real_t &at(std::uint32_t i) const
    {
        assert(rank() == 1);
        assert(i < m_shape[0]);
        return (*this)(i);
    }

    [[nodiscard]] std::uint32_t size1() const
    {
        return m_shape[0];
    }
};

} // namespace job::ai::base
