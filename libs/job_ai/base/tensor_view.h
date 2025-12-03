// tensor_view.h
#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <numeric>
#include <vector>
#include <type_traits>

#include <real_type.h>
#include "tensor_shape.h"
#include "tensor_iter.h"

namespace job::ai::base {

template<typename T>
class TensorIter;

template<typename T>
class TensorView {
public:
    using value_type = T;
    using Shape      = TensorShape<4>;

    TensorView() = default;

    TensorView(T *data, const Shape &shape) :
        m_data(data),
        m_shape(shape)
    {
        calculateStrides();
    }

    TensorView(T *data, const std::vector<std::uint32_t> &shape) :
        m_data(data),
        m_shape(shape.begin(), shape.end())
    {
        calculateStrides();
    }

    [[nodiscard]] T *data() const noexcept
    {
        return m_data;
    }

    [[nodiscard]] const Shape &shape() const noexcept
    {
        return m_shape;
    }

    [[nodiscard]] std::uint32_t rank() const noexcept
    {
        return static_cast<std::uint32_t>(m_shape.size());
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        if (m_shape.empty())
            return 0;
        return std::accumulate(m_shape.begin(), m_shape.end(),
                               std::size_t{1}, std::multiplies<std::size_t>());
    }

    [[nodiscard]] T &operator[](std::size_t i)
    {
        assert(i < size());
        return m_data[i];
    }

    [[nodiscard]] const T &operator[](std::size_t i) const
    {
        assert(i < size());
        return m_data[i];
    }

    // Generic N-D access (runtime checking)
    template<typename... Indices>
        requires (std::conjunction_v<std::is_integral<Indices>...>)
    [[nodiscard]] T &operator()(Indices... indices)
    {
        assert(sizeof...(indices) == rank());
        std::size_t offset = 0;
        std::size_t dim    = 0;
        ((offset += static_cast<std::size_t>(indices) * m_strides[dim++]), ...);
        assert(offset < size());
        return m_data[offset];
    }

    template<typename... Indices>
        requires (std::conjunction_v<std::is_integral<Indices>...>)
    [[nodiscard]] const T &operator()(Indices... indices) const
    {
        assert(sizeof...(indices) == rank());
        std::size_t offset = 0;
        std::size_t dim    = 0;
        ((offset += static_cast<std::size_t>(indices) * m_strides[dim++]), ...);
        assert(offset < size());
        return m_data[offset];
    }

    [[nodiscard]] TensorView<T> reshape(const Shape &newShape) const
    {
        assert(volume(newShape) == size());
        return TensorView<T>(m_data, newShape);
    }

    [[nodiscard]] TensorView<T> slice(std::uint32_t index) const
    {
        assert(rank() > 0 && index < m_shape[0]);

        // New shape is original shape without the first dimension
        Shape newShape(m_shape.begin() + 1, m_shape.end());

        T *newData = m_data + index * m_strides[0];
        return TensorView<T>(newData, newShape);
    }

    // Slice-iterator over first dimension
    [[nodiscard]] TensorIter<T> begin() const
    {
        return beginSlices(*this);
    }

    [[nodiscard]] TensorIter<T> end() const
    {
        return endSlices(*this);
    }

protected:
    T    *m_data{nullptr};
    Shape m_shape;
    Shape m_strides;

    void calculateStrides()
    {
        if (m_shape.empty())
            return;

        std::uint32_t stride = 1;
        // Row-major: last dimension changes fastest
        for (int i = static_cast<int>(m_shape.size()) - 1; i >= 0; --i) {
            m_strides[i] = stride;
            stride *= m_shape[i];
        }
    }

    static std::size_t volume(const Shape &shape)
    {
        return std::accumulate(shape.begin(), shape.end(),
                               std::size_t{1}, std::multiplies<std::size_t>());
    }
};

using TensorViewR      = TensorView<job::core::real_t>;
using TensorViewConstR = TensorView<const job::core::real_t>;

} // namespace job::ai::base
