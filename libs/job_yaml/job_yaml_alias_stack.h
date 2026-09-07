#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace job::yaml {

class YamlAliasStack
{
public:
    [[nodiscard]] bool push(std::string_view name)
    {
        if (name.empty())
            return false;

        if (contains(name))
            return false;

        m_stack.push_back(name);
        return true;
    }

    void pop() noexcept
    {
        if (!m_stack.empty())
            m_stack.pop_back();
    }

    [[nodiscard]] bool contains(std::string_view name) const noexcept
    {
        for (const std::string_view entry : m_stack) {
            if (entry == name)
                return true;
        }

        return false;
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_stack.size();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_stack.empty();
    }

    void clear() noexcept
    {
        m_stack.clear();
    }

private:
    std::vector<std::string_view> m_stack;
};

} // namespace job::yaml