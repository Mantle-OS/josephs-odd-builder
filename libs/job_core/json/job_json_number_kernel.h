#pragma once

#include <charconv>
#include <string_view>
#include <system_error>

#include "job_json_concepts.h"

/*
    There are two important correctness properties hidden in this tiny code.

    We parse into a temporary first:
    T parsed{};
    and only do: result = parsed; // on complete success.
    So: std::uint8_t value = 42;
    JsonNumberKernel::parse("9999", value);
    fails without "trashing" value.
*/

namespace job::json {

class JsonNumberKernel
{
public:
    template <JsonInteger T>
    [[nodiscard]] static bool parseInteger(std::string_view value, T &result) noexcept
    {
        if (value.empty())
            return false;

        const char *begin = value.data();
        const char *end = begin + value.size();

        T parsed{};
        const auto [ptr, error] = std::from_chars(begin, end, parsed, 10);
        if (error != std::errc{} || ptr != end)
            return false;

        result = parsed;
        return true;
    }

    template <JsonFloatingPoint T>
    [[nodiscard]] static bool parseFloat(std::string_view value, T &result) noexcept
    {
        if (value.empty())
            return false;

        const char *begin = value.data();
        const char *end = begin + value.size();

        T parsed{};
        const auto [ptr, error] = std::from_chars(begin, end,
                                                  parsed,
                                                  std::chars_format::general);

        if (error != std::errc{} || ptr != end)
            return false;

        result = parsed;
        return true;
    }

    template <JsonNumber T>
    [[nodiscard]] static bool parse(std::string_view value, T &result) noexcept
    {
        if constexpr (JsonInteger<T>)
            return parseInteger(value, result);
        else
            return parseFloat(value, result);
    }
};

} // namespace job::json

