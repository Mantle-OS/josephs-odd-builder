#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

#include "job_render_format.h"

namespace job::render {

template <typename T>
concept RenderSurface = requires(T surface, const T constSurface) {
    { constSurface.width() } -> std::convertible_to<std::uint32_t>;
    { constSurface.height() } -> std::convertible_to<std::uint32_t>;

    { constSurface.format() } -> std::same_as<FormatType>;

    { constSurface.stride() } -> std::convertible_to<std::size_t>;
    { constSurface.size() } -> std::convertible_to<std::size_t>;

    { surface.data() } -> std::same_as<std::span<std::byte>>;
    { constSurface.data() } -> std::same_as<std::span<const std::byte>>;

    { constSurface.valid() } -> std::same_as<bool>;
};

} // namespace job::render