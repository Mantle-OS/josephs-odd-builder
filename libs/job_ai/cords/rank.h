#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "view.h"

namespace job::ai::cords {

template<std::size_t R>
concept ValidRank = (R >= 1 && R <= 4);

template<std::size_t RankSize>
    requires ValidRank<RankSize>
class Rank : public ViewR {
public:
    using View  = ViewR;
    using Extent = View::Extent;
    using Indices = std::array<std::uint32_t, RankSize>;

    Rank(float *data, const Indices &indices) :
        View(data, Extent(indices.begin(), indices.end()))
    {
        assert(rank() == RankSize && "Rank mismatch in constructor!");
    }
    // Constructor B: Raw Data + Extent Object
    Rank(float *data, const Extent &ext) :
        View(data, ext)
    {
        assert(rank() == RankSize && "Rank mismatch! Extent does not match Template Rank.");
    }


    [[nodiscard]] float &at(const Indices &indices)
    {
        assert(rank() == RankSize);
        for (std::size_t i = 0; i < RankSize; ++i)
            assert(indices[i] < m_extent[i] && "Rank Extent is smaller than Indices" );

        return accessImpl(indices, std::make_index_sequence<RankSize>{});
    }

    [[nodiscard]] const float &at(const Indices &indices) const
    {
        assert(rank() == RankSize);
        for (std::size_t i = 0; i < RankSize; ++i)
            assert(indices[i] < m_extent[i] && "Rank Extent is smaller than Indices");

        return accessImpl(indices, std::make_index_sequence<RankSize>{});
    }

    // Usage: my_rank.at(batch, seq, dim)
    template<typename... Args>
        requires (sizeof...(Args) == RankSize)
    [[nodiscard]] float &at(Args... args)
    {
        return (*this)(static_cast<std::uint32_t>(args)...);
    }

    template<typename... Args>
        requires (sizeof...(Args) == RankSize)
    [[nodiscard]] const float &at(Args... args) const
    {
        return (*this)(static_cast<std::uint32_t>(args)...);
    }

private:
    template<std::size_t... Indice>
    [[nodiscard]] float &accessImpl(const Indices &indices, std::index_sequence<Indice...>)
    {
        return (*this)(indices[Indice]...);
    }

    template<std::size_t... Indice>
    [[nodiscard]] const float &accessImpl(const Indices &indices, std::index_sequence<Indice...>) const
    {
        return (*this)(indices[Indice]...);
    }
};

// These are dumb as we have full classes for them but whatever
using FiberRank     = Rank<1>;
using MatrixRank    = Rank<2>;
using VolumeRank    = Rank<3>;
using BatchRank     = Rank<4>;

} // namespace job::ai::cords
