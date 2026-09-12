#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "jobusb_export.h"
namespace job::usb {

class JOBUSB_EXPORT JobUsbIdLineKernel
{
public:
    JobUsbIdLineKernel() = delete;
    ~JobUsbIdLineKernel() = delete;

    JobUsbIdLineKernel(const JobUsbIdLineKernel &) = delete;
    JobUsbIdLineKernel &operator=(const JobUsbIdLineKernel &) = delete;
    JobUsbIdLineKernel(JobUsbIdLineKernel &&) = delete;
    JobUsbIdLineKernel &operator=(JobUsbIdLineKernel &&) = delete;

    struct Field
    {
        std::string_view key;
        std::string_view value;
    };

    template <std::unsigned_integral T>
    [[nodiscard]] static bool parseHex(std::string_view text, T &value) noexcept
    {
        if (text.empty())
            return false;

        T parsed = 0;

        const auto result = std::from_chars(text.data(),
                                            text.data() + text.size(),
                                            parsed,
                                            16);

        if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
            return false;

        value = parsed;
        return true;
    }

    [[nodiscard]] static constexpr std::size_t indentation(std::string_view line) noexcept
    {
        std::size_t depth = 0;
        while (depth < line.size() && line[depth] == '\t')
            ++depth;

        return depth;
    }

    [[nodiscard]] static constexpr std::string_view removeIndentation(std::string_view line) noexcept
    {
        const auto depth = indentation(line);
        return line.subview(depth);
    }

    [[nodiscard]] static constexpr std::string_view trimLeft(std::string_view text) noexcept
    {
        while (!text.empty() && isWhitespace(text.front()))
            text.remove_prefix(1);

        return text;
    }

    [[nodiscard]] static constexpr std::string_view trimRight(std::string_view text) noexcept
    {
        while (!text.empty() && isWhitespace(text.back()))
            text.remove_suffix(1);

        return text;
    }

    [[nodiscard]] static constexpr std::string_view trim(std::string_view text) noexcept
    {
        return trimRight(trimLeft(text));
    }

    [[nodiscard]] static constexpr bool blank(std::string_view line) noexcept
    {
        return trim(line).empty();
    }

    [[nodiscard]] static constexpr bool comment(std::string_view line) noexcept
    {
        const auto text = trimLeft(line);
        return !text.empty() && text.front() == '#';
    }

    [[nodiscard]] static constexpr bool splitField(std::string_view text, Field &field) noexcept
    {
        text = trim(text);
        if (text.empty())
            return false;

        std::size_t offset = 0;
        while (offset < text.size() && !isWhitespace(text[offset]))
            ++offset;

        field.key = text.subview(0, offset);

        while (offset < text.size() && isWhitespace(text[offset]))
            ++offset;

        field.value = text.subview(offset);

        return !field.key.empty();
    }

private:
    [[nodiscard]] static constexpr bool isWhitespace(char value) noexcept
    {
        return value == ' ' || value == '\t' || value == '\r' || value == '\n';
    }
};

} // namespace job::usb