#pragma once

#include <string>
#include <cstdint>
#include <cstddef>

namespace job::zstd::test {
    [[nodiscard]] std::string truncate(const std::string &validBytes, double fraction);
    [[nodiscard]] std::string flipBit(const std::string &validBytes, std::size_t byteOffset);
    [[nodiscard]] std::string wireStringWithLie(std::uint32_t claimedLength, const std::string &actualPayload);
} // namespace job::zstd::test