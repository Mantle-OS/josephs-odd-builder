#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <vector>
#include <type_traits>
#include <array>


#include "extents.h"
#include "view_iter.h"

namespace job::ai::cords {

template<typename T_View>
class ViewIter;

template<typename T_View>
class View {
public:
    using value_type  = T_View;
    using Extent      = Extents<4>;

    View() = default;

    View(T_View *view, const Extent &extent) :
        m_view(view),
        m_extent(extent)
    {
        calculateStrides();
    }

    View(T_View *view, const std::vector<std::uint32_t> &extent) :
        m_view(view),
        m_extent(extent.begin(), extent.end())
    {
        calculateStrides();
    }

    [[nodiscard]] T_View *view() const noexcept
    {
        return m_view;
    }

    [[nodiscard]] T_View *data() const noexcept
    {
        return m_view;
    }

    [[nodiscard]] const Extent &extent() const noexcept
    {
        return m_extent;
    }

    [[nodiscard]] std::size_t rank() const noexcept
    {
        return m_extent.size();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_extent.volume();
    }

    [[nodiscard]] T_View &operator[](std::size_t view)
    {
        assert(view < size());
        return m_view[view];
    }

    [[nodiscard]] const T_View &operator[](std::size_t view) const
    {
        assert(view < size());
        return m_view[view];
    }

    // Generic N-D access (runtime checking)
    template<typename... Indices>
        requires (std::conjunction_v<std::is_integral<Indices>...>)
    [[nodiscard]] T_View &operator()(Indices... indices)
    {
        // Must match rank
        assert(sizeof...(indices) == rank());

        std::size_t offset = 0;
        std::size_t dim    = 0;

        //  check dim < rank
        //  check 0 <= index < extent[dim]
        //  accumulate offset += index * stride[dim]
        ([&]{
            const auto idx = static_cast<std::size_t>(indices);

            assert(dim < m_extent.size());
            assert(idx < static_cast<std::size_t>(m_extent[dim]));

            offset += idx * static_cast<std::size_t>(m_strides[dim]);
            ++dim;
        }(), ...);

        assert(offset < size());
        return m_view[offset];
    }

    template<typename... Indices>
        requires (std::conjunction_v<std::is_integral<Indices>...>)
    [[nodiscard]] const T_View &operator()(Indices... indices) const
    {
        assert(sizeof...(indices) == rank());

        std::size_t offset = 0;
        std::size_t dim    = 0;

        ([&]{
            const auto idx = static_cast<std::size_t>(indices);

            assert(dim < m_extent.size());
            assert(idx < static_cast<std::size_t>(m_extent[dim]));

            offset += idx * static_cast<std::size_t>(m_strides[dim]);
            ++dim;
        }(), ...);

        assert(offset < size());
        return m_view[offset];
    }


    [[nodiscard]] View<T_View> reshape(const Extent &newExtent) const
    {
        assert(newExtent.volume() == size());
        return View<T_View>(m_view, newExtent);
    }

    [[nodiscard]] View<T_View> slice(std::uint32_t index) const
    {
        assert(rank() > 0 && index < m_extent[0]);

        Extent newExtent(m_extent.begin() + 1, m_extent.end());

        T_View *newData = m_view + index * m_strides[0];
        return View<T_View>(newData, newExtent);
    }

    [[nodiscard]] std::size_t stride(size_t dim) const
    {
        assert(dim < rank());
        return m_strides[dim];
    }

    // Slice-iterator over first dimension
    [[nodiscard]] ViewIter<T_View> begin() const
    {
        return beginSlices(*this);
    }

    [[nodiscard]] ViewIter<T_View> end() const
    {
        return endSlices(*this);
    }

protected:
    T_View                          *m_view{nullptr};
    Extent                          m_extent;
    std::array<std::size_t, 4>      m_strides{};

    void calculateStrides()
    {
        if (m_extent.empty())
            return;

        std::uint32_t stride = 1;

        // Row-major: last dimension changes fastest
        // iterate backwards from the last dimension
        for (int i = static_cast<int>(m_extent.size()) - 1; i >= 0; --i) {
            m_strides[i] = stride;
            stride *= m_extent[i];
        }
    }
};

using ViewR      = View<float>;
using ViewConstR = View<const float>;

} // namespace job::ai::cords
