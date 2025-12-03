#pragma once

#include <cstdint>
#include <iterator>
// #include "tensor_view_fwd.h" // or just forward declare template<class T> class TensorView;

namespace job::ai::base {

template<typename T>
class TensorView;

template<typename T>
class TensorIter {
public:
    using value_type        = TensorView<T>;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    TensorIter(const TensorView<T> &parent, std::uint32_t index) :
        m_parent(parent),
        m_index(index)
    {
    }

    value_type operator*() const
    {
        return m_parent.slice(m_index);
    }

    TensorIter &operator++()
    {
        ++m_index;
        return *this;
    }

    [[nodiscard]] bool operator!=(const TensorIter &other) const
    {
        return m_index != other.m_index;
    }

private:
    TensorView<T> m_parent;  // lightweight copy of view
    std::uint32_t m_index;
};

template<typename T>
[[nodiscard]] TensorIter<T> beginSlices(const TensorView<T> &v)
{
    return TensorIter<T>(v, 0);
}

template<typename T>
[[nodiscard]] TensorIter<T> endSlices(const TensorView<T> &v)
{
    const auto outer = v.rank() > 0 ? v.shape()[0] : 0;
    return TensorIter<T>(v, outer);
}

} // namespace job::ai::base
