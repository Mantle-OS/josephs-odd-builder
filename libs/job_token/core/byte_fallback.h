#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "job_token_types.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT ByteFallback
{
public:
    [[nodiscard]] static constexpr bool isByteToken(std::string_view text) noexcept
    {
        return text.size() == 6 &&
               text[0] == '<' &&
               text[1] == '0' &&
               text[2] == 'x' &&
               isHexChar(text[3]) &&
               isHexChar(text[4]) &&
               text[5] == '>';
    }

    [[nodiscard]] static constexpr bool parseByteToken(std::string_view text, std::uint8_t &outByte) noexcept
    {
        if (!isByteToken(text))
            return false;

        const std::uint8_t high = static_cast<std::uint8_t>(hexToNibble(text[3]));
        const std::uint8_t low = static_cast<std::uint8_t>(hexToNibble(text[4]));
        outByte = static_cast<std::uint8_t>((high << 4) | low);
        return true;
    }

    [[nodiscard]] static constexpr std::string_view byteToStringView(std::uint8_t byte) noexcept
    {
        return std::string_view{
            kByteHexStorage.storage[byte],
            6
        };
    }

    [[nodiscard]] static std::string formatByte(std::uint8_t byte)
    {
        return std::string{byteToStringView(byte)};
    }

private:
    [[nodiscard]] static constexpr bool isHexChar(char c) noexcept
    {
        return (c >= '0' && c <= '9') ||
               (c >= 'A' && c <= 'F') ||
               (c >= 'a' && c <= 'f');
    }

    [[nodiscard]] static constexpr int hexToNibble(char c) noexcept
    {
        if (c >= '0' && c <= '9')
            return c - '0';

        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;

        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;

        return -1;
    }
};

} // namespace job::token