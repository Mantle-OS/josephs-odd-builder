#pragma once

#include <cstdint>
#include <iterator>

namespace job::ai::cords {

template<typename T_View>
class View;

template<typename T_View>
class ViewIter {
public:
    using value_type        = View<T_View>;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;

    ViewIter() = default;
    ViewIter(const View<T_View> &parent, std::uint32_t index) :
        m_parent(parent),
        m_index(index)
    {
    }

    value_type operator*() const
    {
        return m_parent.slice(m_index);
    }

    ViewIter &operator++()
    {
        ++m_index;
        return *this;
    }

    [[nodiscard]] bool operator==(const ViewIter &other) const
    {
        return m_index == other.m_index &&
               m_parent.view() == other.m_parent.view();
    }

    [[nodiscard]] bool operator!=(const ViewIter &other) const
    {
        return !(*this == other);
    }

private:
    View<T_View>    m_parent;  // lightweight copy of view
    std::uint32_t   m_index;
};

template<typename T_View>
[[nodiscard]] ViewIter<T_View> beginSlices(const View<T_View> &view)
{
    return ViewIter<T_View>(view, 0);
}

template<typename T_View>
[[nodiscard]] ViewIter<T_View> endSlices(const View<T_View> &view)
{
    const auto outer = view.rank() > 0 ? view.extent()[0] : 0;
    return ViewIter<T_View>(view, outer);
}

} // namespace job::ai::cords
