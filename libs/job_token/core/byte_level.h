#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT ByteLevel
{
public:
    [[nodiscard]] static std::string encode(std::string_view input);
    [[nodiscard]] static std::string decode(std::string_view input);

private:
    static void appendUtf8(char32_t cp, std::string &out);
    [[nodiscard]] static bool readUtf8(std::string_view input, std::size_t &offset, char32_t &cp);

    [[nodiscard]] static const std::array<char32_t, 256> &byteToUnicodeTable();
    [[nodiscard]] static const std::unordered_map<char32_t, uint8_t> &unicodeToByteTable();
};

} // namespace job::token