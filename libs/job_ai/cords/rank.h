#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "view.h"
#include <real_type.h>

namespace job::ai::cords {

template<std::size_t R>
concept ValidRank = (R >= 1 && R <= 4);

template<std::size_t RankSize>
class Rank : public ViewR {
public:
    using AView  = ViewR;
    using Extent = AView::Extent;
    using AView::AView;
    using Indices = std::array<std::uint32_t, RankSize>;

    Rank(core::real_t *data, const Indices &indices) :
        AView(data, Extent(indices.begin(), indices.end()))
    {
        assert(rank() == RankSize);
    }

    [[nodiscard]] core::real_t &at(const Indices &indices)
    {
        assert(rank() == RankSize);
        for (std::size_t i = 0; i < RankSize; ++i)
            assert(indices[i] < m_extent[i]);

        return accessImpl(indices, std::make_index_sequence<RankSize>{});
    }

    [[nodiscard]] const core::real_t &at(const Indices &indices) const
    {
        assert(rank() == RankSize);
        for (std::size_t i = 0; i < RankSize; ++i)
            assert(indices[i] < m_extent[i]);

        return accessImpl(indices, std::make_index_sequence<RankSize>{});
    }

private:
    template<std::size_t... Indice>
    [[nodiscard]] core::real_t &accessImpl(const Indices &indices, std::index_sequence<Indice...>)
    {
        return (*this)(indices[Indice]...);
    }

    template<std::size_t... Indice>
    [[nodiscard]] const core::real_t &accessImpl(const Indices &indices, std::index_sequence<Indice...>) const
    {
        return (*this)(indices[Indice]...);
    }
};

using FiberRank     = Rank<1>;
using MatrixRank    = Rank<2>;
using VolumeRank    = Rank<3>;
using BatchRank     = Rank<4>;

} // namespace job::ai::cords
