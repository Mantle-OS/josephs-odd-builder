#include "job_zstd_wire.h"
#include <cstddef>
namespace job::zstd::utils {

void writeU8(std::ostream &out, std::uint8_t value)
{
    out.put(static_cast<char>(value));
}

void writeU64(std::ostream &out, std::uint64_t value)
{
    char bytes[8];
    for (int i = 0; i < 8; ++i) {
        bytes[i] = static_cast<char>(value & 0xFF);
        value >>= 8;
    }
    out.write(bytes, sizeof(bytes));
}

void writeString(std::ostream &out, const std::string &value)
{
    std::uint32_t const length = static_cast<std::uint32_t>(value.size());
    char lenBytes[4];
    std::uint32_t l = length;
    for (int i = 0; i < 4; ++i) {
        lenBytes[i] = static_cast<char>(l & 0xFF);
        l >>= 8;
    }
    out.write(lenBytes, sizeof(lenBytes));
    if (!value.empty())
        out.write(value.data(), static_cast<std::streamsize>(value.size()));
}

bool readU8(std::istream &in, std::uint8_t &value)
{
    char c;
    if (!in.get(c))
        return false;
    value = static_cast<std::uint8_t>(c);
    return true;
}

bool readU64(std::istream &in, std::uint64_t &value)
{
    char bytes[8];
    if (!in.read(bytes, sizeof(bytes)))
        return false;

    value = 0;
    for (int i = 7; i >= 0; --i)
        value = (value << 8) | static_cast<std::uint8_t>(bytes[i]);

    return true;
}

bool readString(std::istream &in, std::string &value)
{
    char lenBytes[4];
    if (!in.read(lenBytes, sizeof(lenBytes)))
        return false;

    std::uint32_t length = 0;
    for (int i = 3; i >= 0; --i)
        length = (length << 8) | static_cast<std::uint8_t>(lenBytes[i]);

    if (length > kMaxWireStringLength)
        return false; // bad length prefix, refused before resize()

    value.resize(length);
    if (length > 0 && !in.read(value.data(), static_cast<std::streamsize>(length)))
        return false;

    return true;
}
} // namespace job::zstd::utils