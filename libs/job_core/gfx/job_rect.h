#pragma once

#include <cstdint>

namespace job::core {
struct JobRect
{
    std::int32_t x1{};
    std::int32_t y1{};
    std::int32_t x2{};
    std::int32_t y2{};

    [[nodiscard]] constexpr std::int32_t width() const noexcept
    {
        return x2 - x1;
    }

    [[nodiscard]] constexpr std::int32_t height() const noexcept
    {
        return y2 - y1;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return x2 > x1 &&
               y2 > y1;
    }

    [[nodiscard]] friend constexpr bool operator==(const JobRect &, const JobRect &) noexcept = default;
};


struct JobRectF
{
    float x1{};
    float y1{};
    float x2{};
    float y2{};

    [[nodiscard]] constexpr float width() const noexcept
    {
        return x2 - x1;
    }

    [[nodiscard]] constexpr float height() const noexcept
    {
        return y2 - y1;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return x2 > x1 &&
               y2 > y1;
    }

    [[nodiscard]] friend constexpr bool operator==(const JobRectF &, const JobRectF &) noexcept = default;
};



}