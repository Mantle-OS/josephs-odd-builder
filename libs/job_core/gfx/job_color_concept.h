#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

#include "job_color_channel.h"
#include "job_color_type.h"
#include "job_render_format_type.h"

namespace job::core {

enum class JobColorType : std::unit8_t {
    Rgba,
    Rgba16,
    Hsv,
    Hsl,
    Cmyk,
    NCol,
    Hwb,
    Invalid
};

// Must have the two types of constructors (r, g, b, a)

template <typename T>
concept JobColorConcept = requires {
    typename std::remove_cvref_t<T>::Channel;
    typename std::remove_cvref_t<T>::Size;

    requires std::unsigned_integral<typename std::remove_cvref_t<T>::Channel>;
} && requires(const std::remove_cvref_t<T> &color, JobColorChannel channel, FormatType format, std::string_view value) {

    { std::remove_cvref_t<T>::maxChannels() }       -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
    { std::remove_cvref_t<T>::maxSize() }           -> std::same_as<typename std::remove_cvref_t<T>::Size>;
    { color.hasChannel(channel) }                   -> std::same_as<bool>;
    { color.channel(channel) }                      -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
    { color.channelMaximum(channel) }               -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
    { color.alpha() }                               -> std::same_as<typename std::remove_cvref_t<T>::Channel>;
    { color.opaque() }                              -> std::same_as<bool>;
    { color.transparent() }                         -> std::same_as<bool>;
    { color.type() }                                -> std::same_as<JobColorType>;
    { color.toString() }                            -> std::same_as<std::string>;
    { std::remove_cvref_t<T>::fromString(value) }   -> std::same_as<std::remove_cvref_t<T>>;
    { color.supports(format) }                      -> std::same_as<bool>;
    { color.valid() }                               -> std::same_as<bool>;
};

}