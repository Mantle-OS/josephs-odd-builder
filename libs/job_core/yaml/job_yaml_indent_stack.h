#pragma once

#include <cstddef>
#include <cstdint>
#include <inplace_vector>

namespace job::yaml {

enum class YamlIndentRelation : std::uint8_t {
    Same,
    Deeper,
    Shallower
};

class YamlIndentStack
{
public:
    using Indent = std::size_t;

    static constexpr std::size_t MaxDepth = 64;

    constexpr bool empty() const noexcept
    {
        return m_stack.empty();
    }

    constexpr std::size_t depth() const noexcept
    {
        return m_stack.size();
    }

    constexpr Indent current() const noexcept
    {
        return m_stack.empty() ? 0 : m_stack.back();
    }

    constexpr YamlIndentRelation relation(Indent indent) const noexcept
    {
        const Indent currentIndent = current();

        if (indent > currentIndent)
            return YamlIndentRelation::Deeper;

        if (indent < currentIndent)
            return YamlIndentRelation::Shallower;

        return YamlIndentRelation::Same;
    }

    constexpr bool contains(Indent indent) const noexcept
    {
        for (const Indent value : m_stack) {
            if (value == indent)
                return true;
        }

        return false;
    }

    constexpr bool push(Indent indent) noexcept
    {
        if (m_stack.size() == MaxDepth)
            return false;

        if (!m_stack.empty() && indent <= m_stack.back())
            return false;

        m_stack.push_back(indent);
        return true;
    }

    constexpr bool pop() noexcept
    {
        if (m_stack.empty())
            return false;

        m_stack.pop_back();
        return true;
    }

    constexpr bool popTo(Indent indent) noexcept
    {
        if (indent != 0 && !contains(indent))
            return false;

        while (!m_stack.empty() && m_stack.back() > indent)
            m_stack.pop_back();

        return m_stack.empty() ? indent == 0 : m_stack.back() == indent;
    }

    constexpr void clear() noexcept
    {
        m_stack.clear();
    }

private:
    std::inplace_vector<Indent, MaxDepth> m_stack;
};

} // namespace job::yaml