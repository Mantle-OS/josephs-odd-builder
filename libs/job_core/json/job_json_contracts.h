#pragma once

#include <cstddef>
#include <string_view>

    namespace job::json {

    [[nodiscard]] constexpr bool validSourceOffset(std::string_view source, std::size_t offset) noexcept
    {
        return offset <= source.size();
    }

    [[nodiscard]] constexpr bool validSourceRange( std::string_view source, std::size_t begin, std::size_t end) noexcept
    {
        return begin <= end && end <= source.size();
    }

    [[nodiscard]] constexpr bool validDepth(std::size_t depth, std::size_t maxDepth) noexcept
    {
        return depth <= maxDepth;
    }

} // namespace job::json

