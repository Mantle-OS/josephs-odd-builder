#include "transient_test_corruption.h"
#include <algorithm>

namespace job::zstd::test {

std::string truncate(const std::string &validBytes, double fraction)
{
    double const clamped = std::clamp(fraction, 0.0, 1.0);
    std::size_t const keep = static_cast<std::size_t>(static_cast<double>(validBytes.size()) * clamped);

    return validBytes.substr(0, keep);
}

std::string flipBit(const std::string &validBytes, std::size_t byteOffset)
{
    if (validBytes.empty())
        return validBytes;

    std::string mangled = validBytes;
    std::size_t const clampedOffset = std::min(byteOffset, mangled.size() - 1);

    mangled[clampedOffset] = static_cast<char>(static_cast<unsigned char>(mangled[clampedOffset]) ^ 0x01);

    return mangled;
}

std::string wireStringWithLie(std::uint32_t claimedLength, const std::string &actualPayload)
{
    std::string result;
    result.reserve(4 + actualPayload.size());

    std::uint32_t length = claimedLength;
    for (int i = 0; i < 4; ++i) {
        result.push_back(static_cast<char>(length & 0xFF));
        length >>= 8;
    }

    result += actualPayload;
    return result;
}

} // namespace job::zstd::test