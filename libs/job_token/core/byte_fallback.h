#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <array>

#include "job_tokenizer_types.h"
#include "jobtoken_export.h"

namespace job::token {

namespace detail {
struct ByteStringTable {
    char storage[256][7]{};

    constexpr ByteStringTable() noexcept {
        constexpr char hexDigits[] = "0123456789ABCDEF";
        for (size_t i = 0; i < 256; ++i) {
            storage[i][0] = '<';
            storage[i][1] = '0';
            storage[i][2] = 'x';
            storage[i][3] = hexDigits[(i >> 4) & 0x0F];
            storage[i][4] = hexDigits[i & 0x0F];
            storage[i][5] = '>';
            storage[i][6] = '\0';
        }
    }
};
inline constexpr ByteStringTable kByteHexStorage{};
} // namespace detail

class JOBTOKEN_EXPORT ByteFallback {
public:
    // Returns true if text matches "<0xNN>" with valid hexadecimal characters
    [[nodiscard]] static constexpr bool isByteToken(std::string_view text) noexcept
    {
        if (text.size() != 6)
            return false;

        if (text[0] != '<' || text[1] != '0' || text[2] != 'x' || text[5] != '>')
            return false;

        return isHexChar(text[3]) && isHexChar(text[4]);
    }

    // Parses "<0xNN>" into raw byte value (0..255). Returns false on format mismatch.
    [[nodiscard]] static constexpr bool parseByteToken(std::string_view text, uint8_t& outByte) noexcept
    {
        if (!isByteToken(text))
            return false;

        const int hi = hexToNibble(text[3]);
        const int lo = hexToNibble(text[4]);
        if (hi < 0 || lo < 0)
            return false;

        outByte = static_cast<uint8_t>((hi << 4) | lo);
        return true;
    }

    // Returns static zero-allocation string_view for any byte (e.g. 0x0A -> "<0x0A>")
    [[nodiscard]] static constexpr std::string_view byteToStringView(uint8_t byteValue) noexcept
    {
        return std::string_view(detail::kByteHexStorage.storage[byteValue], 6);
    }

    // Formats raw byte into "<0xNN>" std::string
    [[nodiscard]] static std::string formatByte(uint8_t byteValue)
    {
        return std::string(byteToStringView(byteValue));
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