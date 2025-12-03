#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "tensor_view.h"
#include <real_type.h>

namespace job::ai::base {

template<std::size_t R>
concept ValidRank = (R >= 1 && R <= 4);

template<std::size_t Rank>
class TensorRank : public TensorViewR {
public:
    using Base  = TensorViewR;
    using Shape = Base::Shape;
    using Base::Base; // inherit TensorViewR constructors

    TensorRank(core::real_t *data,
               const std::array<std::uint32_t, Rank> &dims)
        : Base(data, Shape(dims.begin(), dims.end()))
    {
        assert(rank() == Rank);
    }

    [[nodiscard]] core::real_t &at(const std::array<std::uint32_t, Rank> &indices)
    {
        assert(rank() == Rank);
        for (std::size_t i = 0; i < Rank; ++i)
            assert(indices[i] < m_shape[i]);

        // Use generic indexing via operator()
        return accessImpl(indices, std::make_index_sequence<Rank>{});
    }

    [[nodiscard]] const core::real_t &at(const std::array<std::uint32_t, Rank> &indices) const
    {
        assert(rank() == Rank);
        for (std::size_t i = 0; i < Rank; ++i)
            assert(indices[i] < m_shape[i]);

        return accessImpl(indices, std::make_index_sequence<Rank>{});
    }

private:
    template<std::size_t... I>
    [[nodiscard]] core::real_t &accessImpl(const std::array<std::uint32_t, Rank> &indices,
                                           std::index_sequence<I...>)
    {
        return (*this)(indices[I]...);
    }

    template<std::size_t... I>
    [[nodiscard]] const core::real_t &accessImpl(const std::array<std::uint32_t, Rank> &indices,
                                                 std::index_sequence<I...>) const
    {
        return (*this)(indices[I]...);
    }
};

using TensorRank1D = TensorRank<1>;
using TensorRank2D = TensorRank<2>;
using TensorRank3D = TensorRank<3>;
using TensorRank4D = TensorRank<4>;

} // namespace job::ai::base
