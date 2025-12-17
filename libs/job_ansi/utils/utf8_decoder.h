#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>
#include <string>
#include <cstddef>


// Utf8Decoder allows both incremental decoding (byte-by-byte)
// and static decoding of full UTF-8 strings or streams.
//
// Invalid sequences return U+FFFD (replacement character).

namespace job::ansi::utils {

struct NormUtf8Result {
    std::size_t bytesRead;
    std::size_t bytesWritten;
};

class Utf8Decoder {
public:
    Utf8Decoder() = default;


    // Encode one Unicode codepoint into UTF-8 and return as std::string
    static std::string encode(char32_t ch);

    // Optionally: append UTF-8 to an output string
    static void encodeTo(char32_t ch, std::string &out);

    // Encode one codepoint to UTF-8 bytes.
    // Returns number of bytes written (1..4). Writes '?' for invalid input.
    [[nodiscard]] static std::size_t encodeTo(char32_t ch, std::span<char> output);

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

    // Decode a UTF-8 byte stream and write codepoints to an output span.
    // Returns number of codepoints written. Invalid sequences emit U+FFFD.
    // If output is too small, stops early.
    [[nodiscard]] static std::size_t decodeUtf8StreamTo(std::string_view bytes,
                                                        std::span<char32_t> output);

    // Normalize arbitrary bytes into a valid-ish UTF-8 byte stream.
    // Invalid sequences become U+FFFD, then UTF-8 encoded (EF BF BD).
    // Returns number of bytes written.
    [[nodiscard]] static NormUtf8Result normalizeUtf8To(std::span<const uint8_t> input,
                                                     std::span<uint8_t> output);

    [[nodiscard]] static std::size_t determineLength(unsigned char firstByte);
    [[nodiscard]] bool isComplete() const;
    [[nodiscard]] static bool validateUtf8Sequence(const unsigned char* data, std::size_t len, char32_t &outChar);
    [[nodiscard]] static bool isValid(std::string_view str);
private:



    std::string m_buffer;
    std::size_t m_expectedLength = 0;

};
}
