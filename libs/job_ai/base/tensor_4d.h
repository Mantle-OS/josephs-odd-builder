#pragma once

#include <cassert>
#include <cstdint>

#include "tensor_view.h"

namespace job::ai::base {

class Tensor4D : public TensorViewR {
public:
    using Base  = TensorViewR;
    using Shape = Base::Shape;
    using Base::Base; // inherit TensorViewR constructors

    Tensor4D(core::real_t *data,
             std::uint32_t b,
             std::uint32_t c,
             std::uint32_t h,
             std::uint32_t w) :
        Base(data, Shape{b, c, h, w})
    {
        assert(rank() == 4);
    }

    [[nodiscard]] core::real_t &at(std::uint32_t b, std::uint32_t c,
                                   std::uint32_t h, std::uint32_t w)
    {
        assert(rank() == 4);
        assert(b < m_shape[0] && c < m_shape[1]
               && h < m_shape[2] && w < m_shape[3]);
        return (*this)(b, c, h, w);
    }

    [[nodiscard]] const core::real_t &at(std::uint32_t b, std::uint32_t c,
                                         std::uint32_t h, std::uint32_t w) const
    {
        assert(rank() == 4);
        assert(b < m_shape[0] && c < m_shape[1]
               && h < m_shape[2] && w < m_shape[3]);
        return (*this)(b, c, h, w);
    }

    [[nodiscard]] std::uint32_t batch() const
    {
        return m_shape[0];
    }
    [[nodiscard]] std::uint32_t channels() const
    {
        return m_shape[1];
    }
    [[nodiscard]] std::uint32_t height() const
    {
        return m_shape[2];
    }
    [[nodiscard]] std::uint32_t width() const
    {
        return m_shape[3];
    }
};

} // namespace job::ai::base
