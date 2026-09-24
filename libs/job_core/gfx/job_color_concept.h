#pragma once

#include <concepts>
#include <type_traits>

#include "job_color_channel.h"

namespace job::render {

enum class JobColorType : std::uint8_t
{
    Rgba = 0,
    Rgba16,
    Hsv,
    Hsl,
    Hwb,
    Cmyk,
    Ncol,
    Invalid // = 7 slash sentanal
};
template <typename T>
concept JobColorConcept =
    requires {
        typename std::remove_cvref_t<T>::Channel;
        typename std::remove_cvref_t<T>::Size;

        typename std::remove_cvref_t<T>::Span; // std::same_as<std::span<std::byte>>;
        requires std::unsigned_integral<typename std::remove_cvref_t<T>::Channel>;
    } &&
    requires(const std::remove_cvref_t<T> &color, JobColorChannel channel, FormatType format, std::string_view value) {

        { std::remove_cvref_t<T>::maxChannels() } -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
        { std::remove_cvref_t<T>::maxSize() } -> std::same_as<typename std::remove_cvref_t<T>::Size>;

        { color.channel(channel) } -> std::same_as<typename std::remove_cvref_t<T>::Channel>;

        { color.red() }   -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
        { color.green() } -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
        { color.blue() }  -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
        { color.alpha() } -> std::same_as<typename std::remove_cvref_t<T>::Channel>;

        { color.opaque() }      -> std::same_as<bool>;
        { color.transparent() } -> std::same_as<bool>;

        { color.type() } -> std::same_as<JobColorType>;

        { color.toString() } -> std::same_as<std::string>;
        { std::remove_cvref_t<T>::fromString(value) } -> std::same_as<std::remove_cvref_t<T>>;

        { color.binary(fomat) } -> std::same_as<std::span<std::byte>>;
        { std::remove_cvref_t<T>::fromBinary(format) } -> std::same_as<std::remove_cvref_t<T>>;

        { color.supports(format) } -> std::same_as<bool>;
        { color.valid() } -> std::same_as<bool>;

        // { color.pack(format) } -> std::same_as<typename std::remove_cvref_t<T>::Size>;
    };
}