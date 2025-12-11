#include "utf8_decoder.h"
namespace job::ansi::utils {
std::string Utf8Decoder::encode(char32_t ch)
{
    std::string result;
    encodeTo(ch, result);
    return result;
}

void Utf8Decoder::encodeTo(char32_t ch, std::string &out)
{
    if (ch <= 0x7F) {
        out.push_back(static_cast<char>(ch));
    } else if (ch <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((ch >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
    } else if (ch <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((ch >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
    } else if (ch <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((ch >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((ch >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
    } else {
        out.push_back('?'); // invalid input fallback
    }
}


char32_t Utf8Decoder::decodeUtf8(std::string_view bytes)
{
    if (bytes.empty() || bytes.size() > 4)
        return U'\uFFFD';

    const auto *s = reinterpret_cast<const unsigned char *>(bytes.data());
    char32_t outChar = 0;

    if (validateUtf8Sequence(s, bytes.size(), outChar))
        return outChar;

    return U'\uFFFD';
}

std::vector<char32_t> Utf8Decoder::decodeUtf8Stream(std::string_view bytes)
{
    std::vector<char32_t> result;
    const auto *s = reinterpret_cast<const unsigned char *>(bytes.data());
    std::size_t i = 0;
    const std::size_t len = bytes.size();

    while (i < len) {
        std::size_t cpLen = determineLength(s[i]);

        if (cpLen == 0 || i + cpLen > len) {
            result.push_back(U'\uFFFD');
            ++i;
            continue;
        }

        char32_t ch = 0;
        if (validateUtf8Sequence(s + i, cpLen, ch)) {
            result.push_back(ch);
            i += cpLen;
        } else {
            result.push_back(U'\uFFFD');
            ++i;
        }
    }

    return result;
}

void Utf8Decoder::appendByte(char byte)
{
    const unsigned char ub = static_cast<unsigned char>(byte);
    if (m_buffer.empty())
        m_expectedLength = determineLength(ub);

    m_buffer.push_back(byte);
}

bool Utf8Decoder::hasChar() const
{
    return isComplete();
}

char32_t Utf8Decoder::takeChar()
{
    char32_t result = U'\uFFFD';

    if (isComplete()) {
        const auto *s = reinterpret_cast<const unsigned char *>(m_buffer.data());
        bool ok = validateUtf8Sequence(s, m_buffer.size(), result);
        if (!ok) {
            // Log or debug-assert if desired
            // std::cerr << "Invalid UTF-8 sequence in buffer\n";
        }
    }

    reset();
    return result;
}

void Utf8Decoder::reset()
{
    m_buffer.clear();
    m_expectedLength = 0;
}

bool Utf8Decoder::isComplete() const
{
    return m_expectedLength > 0 && m_buffer.size() == m_expectedLength;
}

std::size_t Utf8Decoder::determineLength(unsigned char byte)
{
    if ((byte & 0b10000000) == 0x00)
        return 1;

    if ((byte & 0b11100000) == 0xC0)
        return 2;

    if ((byte & 0b11110000) == 0xE0)
        return 3;

    if ((byte & 0b11111000) == 0xF0)
        return 4;

    return 0; // Invalid lead byte
}

bool Utf8Decoder::validateUtf8Sequence(const unsigned char *data, std::size_t len, char32_t &outChar)
{
    switch (len) {
    case 1:
        if (data[0] <= 0x7F) {
            outChar = data[0];
            return true;
        }
        return false;

    case 2:
        if ((data[1] & 0xC0) != 0x80)
            return false;

        outChar = ((data[0] & 0x1F) << 6) |
                  (data[1] & 0x3F);
        return outChar >= 0x80;

    case 3:
        if ((data[1] & 0xC0) != 0x80 || (data[2] & 0xC0) != 0x80)
            return false;

        outChar = ((data[0] & 0x0F) << 12) |
                  ((data[1] & 0x3F) << 6) |
                  (data[2] & 0x3F);
        return outChar >= 0x800;

    case 4:
        if ((data[1] & 0xC0) != 0x80 ||
            (data[2] & 0xC0) != 0x80 ||
            (data[3] & 0xC0) != 0x80)
            return false;

        outChar = ((data[0] & 0x07) << 18) |
                  ((data[1] & 0x3F) << 12) |
                  ((data[2] & 0x3F) << 6) |
                  (data[3] & 0x3F);
        return outChar >= 0x10000 && outChar <= 0x10FFFF;

    default:
        return false;
    }
}

}
