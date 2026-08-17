#include "gpt2_byte_encoder.h"

#include <array>
#include <cstdint>

namespace job::token {

class Gpt2ByteEncoderTable final
{
public:
    static constexpr std::size_t CodePointTableSize = 324;

    using EncodeTable = std::array<char32_t, 256>;
    using DecodeTable = std::array<std::int16_t, CodePointTableSize>;

    [[nodiscard]] static const EncodeTable &encodeTable()
    {
        static const EncodeTable table = createEncodeTable();
        return table;
    }

    [[nodiscard]] static const DecodeTable &decodeTable()
    {
        static const DecodeTable table = createDecodeTable();
        return table;
    }

    [[nodiscard]] static std::string utf8(char32_t codePoint)
    {
        std::string output;

        if (codePoint <= 0x7F) {
            output.push_back(
                static_cast<char>(
                    codePoint));

            return output;
        }

        if (codePoint <= 0x7FF) {
            output.push_back(
                static_cast<char>(
                    0xC0 |
                    ((codePoint >> 6) & 0x1F)));

            output.push_back(
                static_cast<char>(
                    0x80 |
                    (codePoint & 0x3F)));

            return output;
        }

        if (codePoint <= 0xFFFF) {
            output.push_back(
                static_cast<char>(
                    0xE0 |
                    ((codePoint >> 12) & 0x0F)));

            output.push_back(
                static_cast<char>(
                    0x80 |
                    ((codePoint >> 6) & 0x3F)));

            output.push_back(
                static_cast<char>(
                    0x80 |
                    (codePoint & 0x3F)));

            return output;
        }

        output.push_back(
            static_cast<char>(
                0xF0 |
                ((codePoint >> 18) & 0x07)));

        output.push_back(
            static_cast<char>(
                0x80 |
                ((codePoint >> 12) & 0x3F)));

        output.push_back(
            static_cast<char>(
                0x80 |
                ((codePoint >> 6) & 0x3F)));

        output.push_back(
            static_cast<char>(
                0x80 |
                (codePoint & 0x3F)));

        return output;
    }

    [[nodiscard]] static bool nextUtf8(
        std::string_view input,
        std::size_t &offset,
        char32_t &codePoint) noexcept
    {
        if (offset >= input.size())
            return false;

        const auto first =
            static_cast<std::uint8_t>(
                input[offset]);

        if ((first & 0x80) == 0) {
            codePoint = first;
            ++offset;
            return true;
        }

        if ((first & 0xE0) == 0xC0) {
            if (offset + 1 >= input.size())
                return false;

            const auto second =
                static_cast<std::uint8_t>(
                    input[offset + 1]);

            if ((second & 0xC0) != 0x80)
                return false;

            codePoint =
                static_cast<char32_t>(
                    ((first & 0x1F) << 6) |
                    (second & 0x3F));

            offset += 2;
            return true;
        }

        if ((first & 0xF0) == 0xE0) {
            if (offset + 2 >= input.size())
                return false;

            const auto second =
                static_cast<std::uint8_t>(
                    input[offset + 1]);

            const auto third =
                static_cast<std::uint8_t>(
                    input[offset + 2]);

            if ((second & 0xC0) != 0x80 ||
                (third & 0xC0) != 0x80) {
                return false;
            }

            codePoint =
                static_cast<char32_t>(
                    ((first & 0x0F) << 12) |
                    ((second & 0x3F) << 6) |
                    (third & 0x3F));

            offset += 3;
            return true;
        }

        if ((first & 0xF8) == 0xF0) {
            if (offset + 3 >= input.size())
                return false;

            const auto second =
                static_cast<std::uint8_t>(
                    input[offset + 1]);

            const auto third =
                static_cast<std::uint8_t>(
                    input[offset + 2]);

            const auto fourth =
                static_cast<std::uint8_t>(
                    input[offset + 3]);

            if ((second & 0xC0) != 0x80 ||
                (third & 0xC0) != 0x80 ||
                (fourth & 0xC0) != 0x80) {
                return false;
            }

            codePoint =
                static_cast<char32_t>(
                    ((first & 0x07) << 18) |
                    ((second & 0x3F) << 12) |
                    ((third & 0x3F) << 6) |
                    (fourth & 0x3F));

            offset += 4;
            return true;
        }

        return false;
    }

private:
    [[nodiscard]] static constexpr bool isDirectByte(
        std::uint16_t byte) noexcept
    {
        return
            (byte >= 33 && byte <= 126) ||
            (byte >= 161 && byte <= 172) ||
            (byte >= 174 && byte <= 255);
    }

    [[nodiscard]] static EncodeTable createEncodeTable()
    {
        EncodeTable table{};

        char32_t nextCodePoint = 256;

        for (std::uint16_t byte = 0;
             byte < 256;
             ++byte) {

            if (isDirectByte(byte)) {
                table[byte] =
                    static_cast<char32_t>(
                        byte);

                continue;
            }

            table[byte] =
                nextCodePoint;

            ++nextCodePoint;
        }

        return table;
    }

    [[nodiscard]] static DecodeTable createDecodeTable()
    {
        DecodeTable table{};

        table.fill(
            static_cast<std::int16_t>(
                -1));

        const EncodeTable &encode =
            encodeTable();

        for (std::uint16_t byte = 0;
             byte < 256;
             ++byte) {

            const char32_t codePoint =
                encode[byte];

            if (codePoint <
                static_cast<char32_t>(
                    table.size())) {

                table[codePoint] =
                    static_cast<std::int16_t>(
                        byte);
            }
        }

        return table;
    }
};

Gpt2ByteEncoder::Ptr Gpt2ByteEncoder::createShared()
{
    return std::make_shared<Gpt2ByteEncoder>();
}

Gpt2ByteEncoder::UPtr Gpt2ByteEncoder::createUniq()
{
    return std::make_unique<Gpt2ByteEncoder>();
}

ByteSymbols Gpt2ByteEncoder::encode(
    std::string_view input) const
{
    const Gpt2ByteEncoderTable::EncodeTable &table =
        Gpt2ByteEncoderTable::encodeTable();

    ByteSymbols output;

    // One tokenizer symbol is produced for every input byte.
    output.reserve(
        input.size());

    for (const char character : input) {
        const auto byte =
            static_cast<std::uint8_t>(
                character);

        output.emplace_back(
            Gpt2ByteEncoderTable::utf8(
                table[byte]));
    }

    return output;
}

std::string Gpt2ByteEncoder::decode(
    std::span<const std::string> input) const
{
    const Gpt2ByteEncoderTable::DecodeTable &table =
        Gpt2ByteEncoderTable::decodeTable();

    std::string output;

    std::size_t capacity = 0;

    for (const std::string &symbol : input)
        capacity += symbol.size();

    output.reserve(capacity);

    for (const std::string &symbol : input) {
        std::size_t offset = 0;

        while (offset < symbol.size()) {
            char32_t codePoint = 0;

            if (!Gpt2ByteEncoderTable::nextUtf8(
                    symbol,
                    offset,
                    codePoint)) {
                return {};
            }

            if (codePoint >=
                static_cast<char32_t>(
                    table.size())) {
                return {};
            }

            const std::int16_t byte =
                table[codePoint];

            if (byte < 0)
                return {};

            output.push_back(
                static_cast<char>(
                    static_cast<std::uint8_t>(
                        byte)));
        }
    }

    return output;
}

} // namespace job::token