#pragma once

#include <string_view>

namespace job::yaml {

struct YamlNodeProperties
{
    std::string_view anchor{};
    std::string_view tag{};

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return anchor.empty() && tag.empty();
    }

    [[nodiscard]] constexpr bool hasAnchor() const noexcept
    {
        return !anchor.empty();
    }

    [[nodiscard]] constexpr bool hasTag() const noexcept
    {
        return !tag.empty();
    }

    constexpr void clear() noexcept
    {
        anchor = {};
        tag = {};
    }
};

} // namespace job::yaml