#pragma once

#include <string_view>
#include <vector>
#include <string>
#include <cstddef>

/**
 * Utf8Decoder allows both incremental decoding (byte-by-byte)
 * and static decoding of full UTF-8 strings or streams.
 *
 * Invalid sequences return U+FFFD (replacement character).
 */
namespace job::ansi::utils {

class Utf8Decoder {
public:
    Utf8Decoder() = default;

    // Encode one Unicode codepoint into UTF-8 and return as std::string
    static std::string encode(char32_t ch);

    // Optionally: append UTF-8 to an output string
    static void encodeTo(char32_t ch, std::string &out);

    // Append a single byte to the stream
    void appendByte(char byte);

    // Returns true if the current byte sequence is complete and valid
    [[nodiscard]] bool hasChar() const;

    // Extract the decoded character (U+FFFD if invalid), then reset
    [[nodiscard]] char32_t takeChar();

    // Reset internal buffer (e.g., on error)
    void reset();

    // Static decode of a full UTF-8 sequence into a single codepoint
    [[nodiscard]] static char32_t decodeUtf8(std::string_view bytes);

    // Static decode of a full UTF-8 byte stream into a sequence of codepoints
    [[nodiscard]] static std::vector<char32_t> decodeUtf8Stream(std::string_view bytes);

private:
    std::string m_buffer;
    std::size_t m_expectedLength = 0;

    [[nodiscard]] bool isComplete() const;
    [[nodiscard]] static std::size_t determineLength(unsigned char firstByte);
    [[nodiscard]] static bool validateUtf8Sequence(const unsigned char* data, std::size_t len, char32_t &outChar);
};
}
