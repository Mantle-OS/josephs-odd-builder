#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "simd_provider.h"

namespace job::core::text {

template <std::size_t RegisterCount>
class AsciiSimdTile
{
public:
    static_assert(RegisterCount == 1 || RegisterCount == 2 || RegisterCount == 4, "AsciiSimdTile supports 1, 2, or 4 SIMD registers");
    static constexpr std::size_t kRegisterBytes = static_cast<std::size_t>(simd::SIMD::width_u8());
    static constexpr std::size_t kRegisterCount = RegisterCount;
    static constexpr std::size_t kByteCount = kRegisterBytes * kRegisterCount;

    using Input = std::span<const std::uint8_t, kByteCount>;

    AsciiSimdTile() = delete;
    ~AsciiSimdTile() = delete;

    AsciiSimdTile(const AsciiSimdTile &) = delete;
    AsciiSimdTile &operator=(const AsciiSimdTile &) = delete;
    AsciiSimdTile(AsciiSimdTile &&) = delete;
    AsciiSimdTile &operator=(AsciiSimdTile &&) = delete;

    __attribute__((always_inline))
    [[nodiscard]] static inline std::size_t asciiPrefixBytes(Input input) noexcept
    {
        if constexpr (kRegisterCount == 1) {
            return asciiPrefix1(input);
        } else if constexpr (kRegisterCount == 2) {
            return asciiPrefix2(input);
        } else {
            static_assert(kRegisterCount == 4);
            return asciiPrefix4(input);
        }
    }

private:
    __attribute__((always_inline))
    [[nodiscard]] static inline std::size_t asciiPrefix1(Input input) noexcept
    {
        const simd::u8 v0 = simd::SIMD::pull_u8(input.data());
        if (!simd::SIMD::any_high_bit_u8(v0))
            return kByteCount;

        return 0;
    }

    __attribute__((always_inline))
    [[nodiscard]] static inline std::size_t asciiPrefix2(Input input) noexcept
    {
        const simd::u8 v0 = simd::SIMD::pull_u8(input.data());
        const simd::u8 v1 = simd::SIMD::pull_u8(input.data() + kRegisterBytes);

        const simd::u8 combined = simd::SIMD::or_u8(v0, v1);

        if (!simd::SIMD::any_high_bit_u8(combined))
            return kByteCount;

        if (simd::SIMD::any_high_bit_u8(v0))
            return 0;

        return kRegisterBytes;
    }

    __attribute__((always_inline))
    [[nodiscard]] static inline std::size_t asciiPrefix4(Input input) noexcept
    {
        const simd::u8 v0 = simd::SIMD::pull_u8(input.data());
        const simd::u8 v1 = simd::SIMD::pull_u8(input.data() + kRegisterBytes);
        const simd::u8 v2 = simd::SIMD::pull_u8(input.data() + kRegisterBytes * 2);
        const simd::u8 v3 = simd::SIMD::pull_u8(input.data() + kRegisterBytes * 3);

        const simd::u8 p0 = simd::SIMD::or_u8(v0, v1);
        const simd::u8 p1 = simd::SIMD::or_u8(v2, v3);
        const simd::u8 combined = simd::SIMD::or_u8(p0, p1);

        if (!simd::SIMD::any_high_bit_u8(combined))
            return kByteCount;

        if (simd::SIMD::any_high_bit_u8(v0))
            return 0;

        if (simd::SIMD::any_high_bit_u8(v1))
            return kRegisterBytes;

        if (simd::SIMD::any_high_bit_u8(v2))
            return kRegisterBytes * 2;

        return kRegisterBytes * 3;
    }
};

static_assert(AsciiSimdTile<1>::kRegisterBytes > 0);
static_assert(AsciiSimdTile<1>::kByteCount == AsciiSimdTile<1>::kRegisterBytes);
static_assert(AsciiSimdTile<2>::kByteCount == AsciiSimdTile<2>::kRegisterBytes * 2);
static_assert(AsciiSimdTile<4>::kByteCount == AsciiSimdTile<4>::kRegisterBytes * 4);

} // namespace job::core::text