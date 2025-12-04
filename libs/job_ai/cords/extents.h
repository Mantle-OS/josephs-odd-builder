#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

#include <initializer_list>

#include <iterator>
#include <type_traits>

namespace job::ai::cords {

template<std::size_t MaxRank = 4>
class Extents {
public:
    constexpr Extents() :
        m_size(0)
    {
    }

    template<typename... Dims>
        requires (sizeof...(Dims) <= MaxRank)
    constexpr Extents(Dims... dims) :
        m_size(sizeof...(Dims))
    {
        std::size_t i = 0;
        ((m_data[i++] = static_cast<std::uint32_t>(dims)), ...);

        for (; i < MaxRank; ++i)
            m_data[i] = 0;
    }

    template<typename It>
        requires (!std::is_integral_v<It>)
    constexpr Extents(It first, It last)
    {
        const auto count = static_cast<std::size_t>(std::distance(first, last));
        assert(count <= MaxRank);
        m_size = count;

        std::size_t i = 0;
        for (; first != last; ++first, ++i)
            m_data[i] = static_cast<std::uint32_t>(*first);

        for (; i < MaxRank; ++i)
            m_data[i] = 0;
    }


    [[nodiscard]] constexpr std::uint32_t operator[](std::size_t i) const
    {
        return m_data[i];
    }

    [[nodiscard]] constexpr std::uint32_t &operator[](std::size_t i)
    {
        return m_data[i];
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }

    [[nodiscard]] constexpr std::size_t rank() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] constexpr const std::uint32_t *data() const noexcept
    {
        return m_data.data();
    }

    [[nodiscard]] constexpr std::uint32_t *data() noexcept
    {
        return m_data.data();
    }

    [[nodiscard]] constexpr std::size_t volume() const noexcept
    {
        if (m_size == 0)
            return 0;

        if (m_size == 1 && m_data[0] == 0)
            return 0;

        std::size_t v = 1;
        for(std::size_t i=0; i < m_size; ++i)
            v *= m_data[i];
        return v;
    }

    [[nodiscard]] constexpr bool operator==(const Extents &other) const noexcept
    {
        if (m_size != other.m_size)
            return false;

        for(std::size_t i=0; i < m_size; ++i)
            if (m_data[i] != other.m_data[i])
                return false;

        return true;
    }

    [[nodiscard]] constexpr bool operator!=(const Extents &other) const noexcept
    {
        return !(*this == other);
    }

    constexpr auto begin() noexcept
    {
        return m_data.begin();
    }

    constexpr auto end() noexcept
    {
        return m_data.begin() + m_size;
    }

    constexpr auto begin() const noexcept
    {
        return m_data.begin();
    }

    constexpr auto end()   const noexcept
    {
        return m_data.begin() + m_size;
    }

private:
    std::array<std::uint32_t, MaxRank> m_data{};
    std::size_t                        m_size;
};

// FIXME these should be there own classes
using Layout  = Extents<1>;
using Tile4   = Extents<4>;
using Tile8   = Extents<8>;
using Tile6   = Extents<16>;

} // namespace job::ai::cords
