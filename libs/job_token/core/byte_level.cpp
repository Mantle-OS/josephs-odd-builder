#include "byte_level.h"

namespace job::token {

void ByteLevel::appendUtf8(char32_t cp, std::string &out)
{
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

bool ByteLevel::readUtf8(std::string_view input, std::size_t &offset, char32_t &cp)
{
    if (offset >= input.size())
        return false;

    const auto c0 = static_cast<unsigned char>(input[offset]);

    if ((c0 & 0x80) == 0) {
        cp = c0;
        ++offset;
        return true;
    }

    if ((c0 & 0xE0) == 0xC0) {
        if (offset + 1 >= input.size())
            return false;

        const auto c1 = static_cast<unsigned char>(input[offset + 1]);

        cp =
            (static_cast<char32_t>(c0 & 0x1F) << 6) |
            static_cast<char32_t>(c1 & 0x3F);

        offset += 2;
        return true;
    }

    if ((c0 & 0xF0) == 0xE0) {
        if (offset + 2 >= input.size())
            return false;

        const auto c1 = static_cast<unsigned char>(input[offset + 1]);
        const auto c2 = static_cast<unsigned char>(input[offset + 2]);

        cp =
            (static_cast<char32_t>(c0 & 0x0F) << 12) |
            (static_cast<char32_t>(c1 & 0x3F) << 6) |
            static_cast<char32_t>(c2 & 0x3F);

        offset += 3;
        return true;
    }

    if ((c0 & 0xF8) == 0xF0) {
        if (offset + 3 >= input.size())
            return false;

        const auto c1 = static_cast<unsigned char>(input[offset + 1]);
        const auto c2 = static_cast<unsigned char>(input[offset + 2]);
        const auto c3 = static_cast<unsigned char>(input[offset + 3]);

        cp =
            (static_cast<char32_t>(c0 & 0x07) << 18) |
            (static_cast<char32_t>(c1 & 0x3F) << 12) |
            (static_cast<char32_t>(c2 & 0x3F) << 6) |
            static_cast<char32_t>(c3 & 0x3F);

        offset += 4;
        return true;
    }

    return false;
}

const std::array<char32_t, 256> &ByteLevel::byteToUnicodeTable()
{
    static const std::array<char32_t, 256> table = [] {
        std::array<char32_t, 256> result{};
        std::array<bool, 256> used{};

        for (int i = '!'; i <= '~'; ++i)
            used[static_cast<std::size_t>(i)] = true;

        for (int i = 0xA1; i <= 0xAC; ++i)
            used[static_cast<std::size_t>(i)] = true;

        for (int i = 0xAE; i <= 0xFF; ++i)
            used[static_cast<std::size_t>(i)] = true;

        char32_t next = 256;

        for (std::size_t i = 0; i < result.size(); ++i) {
            if (used[i])
                result[i] = static_cast<char32_t>(i);
            else
                result[i] = next++;
        }

        return result;
    }();

    return table;
}

const std::unordered_map<char32_t, uint8_t> &ByteLevel::unicodeToByteTable()
{
    static const std::unordered_map<char32_t, uint8_t> table = [] {
        std::unordered_map<char32_t, uint8_t> result;
        result.reserve(256);

        const auto &forward = byteToUnicodeTable();

        for (std::size_t i = 0; i < forward.size(); ++i)
            result.emplace(forward[i], static_cast<uint8_t>(i));

        return result;
    }();

    return table;
}

std::string ByteLevel::encode(std::string_view input)
{
    const auto &table = byteToUnicodeTable();

    std::string result;
    result.reserve(input.size() * 2);

    for (const unsigned char byte : input)
        appendUtf8(table[byte], result);

    return result;
}

std::string ByteLevel::decode(std::string_view input)
{
    const auto &table = unicodeToByteTable();

    std::string result;
    result.reserve(input.size());

    std::size_t offset = 0;

    while (offset < input.size()) {
        char32_t cp = 0;

        if (!readUtf8(input, offset, cp))
            break;

        const auto it = table.find(cp);

        if (it != table.end())
            result.push_back(static_cast<char>(it->second));
    }

    return result;
}

} // namespace job::token