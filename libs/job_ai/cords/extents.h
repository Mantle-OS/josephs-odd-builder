#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

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
        requires (!std::is_integral_v<It>) // iterator ctor
    constexpr Extents(It first, It last) :
        m_size(0)
    {
        // m_data is already zero-initialized by the class member definition
        while (first != last) {
            if (m_size >= MaxRank)
                throw std::length_error("Extents: too many dimensions");

            m_data[m_size] = static_cast<std::uint32_t>(*first);

            ++m_size;
            ++first;
        }
        // No need to zero-fill the rest; m_data{} did it for us ! Nice !
    }


    [[nodiscard]] constexpr std::uint32_t operator[](std::size_t i) const
    {
        assert(i < m_size && "Extents index out of logical range");
        return m_data[i];
    }

    [[nodiscard]] constexpr std::uint32_t &operator[](std::size_t i)
    {
        assert(i < m_size && "Extents index out of logical range");
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

    constexpr auto end() const noexcept
    {
        return m_data.begin() + m_size;
    }

private:
    std::array<std::uint32_t, MaxRank>  m_data{};
    std::size_t                         m_size;
};

using Extent = Extents<4>;
[[nodiscard]] constexpr Extent makeBS(std::uint32_t B, std::uint32_t S) noexcept
{
    return Extent{B, S};
}

[[nodiscard]] constexpr Extent makeBSD(std::uint32_t B, std::uint32_t S, std::uint32_t D) noexcept
{
    return Extent{B, S, D};
}

[[nodiscard]] constexpr Extent makeBSDH(std::uint32_t B, std::uint32_t S, std::uint32_t D, std::uint32_t H) noexcept
{
    return Extent{B, S, D, H};
}

} // namespace job::ai::cords
