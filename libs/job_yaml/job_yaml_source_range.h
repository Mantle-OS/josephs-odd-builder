#pragma once

#include <cstddef>
#include <string_view>

namespace job::yaml {

struct YamlSourceRange
{
    std::size_t offset{};
    std::size_t size{};

    [[nodiscard]] constexpr std::size_t endOffset() const noexcept
    {
        return offset + size;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size == 0;
    }

    [[nodiscard]] constexpr bool contains(std::size_t sourceOffset) const noexcept
    {
        return sourceOffset >= offset && sourceOffset < endOffset();
    }

    [[nodiscard]] constexpr std::string_view view(std::string_view source) const noexcept
    {
        if (offset > source.size())
            return {};

        const std::size_t available = source.size() - offset;

        if (size > available)
            return {};

        return source.substr(offset, size);
    }
};

} // namespace job::yaml