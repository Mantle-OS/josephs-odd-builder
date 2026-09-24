#pragma once

#include <cstdint>

namespace job::render {

enum class RenderBufferUsage : std::uint32_t
{
    None         = 0,
    CpuRead      = 1U << 0,
    CpuWrite     = 1U << 1,
    RenderTarget = 1U << 2,
    Texture      = 1U << 3,
    Scanout      = 1U << 4,
    Shared       = 1U << 5,
    Linear       = 1U << 6
};

[[nodiscard]] constexpr RenderBufferUsage operator|(RenderBufferUsage lhs, RenderBufferUsage rhs) noexcept
{
    return static_cast<RenderBufferUsage>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

[[nodiscard]] constexpr RenderBufferUsage operator&(RenderBufferUsage lhs, RenderBufferUsage rhs) noexcept
{
    return static_cast<RenderBufferUsage>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
}

constexpr RenderBufferUsage &operator|=(RenderBufferUsage &lhs, RenderBufferUsage rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

constexpr RenderBufferUsage &operator&=(RenderBufferUsage &lhs, RenderBufferUsage rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

[[nodiscard]] constexpr bool hasUsage(RenderBufferUsage value, RenderBufferUsage usage) noexcept
{
    return (value & usage) == usage;
}

[[nodiscard]] constexpr bool anyUsage(RenderBufferUsage value) noexcept
{
    return value != RenderBufferUsage::None;
}

}