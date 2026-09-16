#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "simd_provider.h"

namespace job::core::text {

enum class Utf32SimdTileStatus : std::uint8_t
{
    Valid,
    Invalid
};

template <std::size_t RegisterCount>
class Utf32SimdTile
{
public:
    static_assert(RegisterCount == 1 ||
                  RegisterCount == 2 ||
                  RegisterCount == 4 ||
                  RegisterCount == 8 ||
                  RegisterCount == 16,
                  "Utf32SimdTile supports 1, 2, 4, 8, or 16 SIMD registers");

    using Status = Utf32SimdTileStatus;

    static constexpr std::size_t kRegisterUnits = static_cast<std::size_t>(simd::SIMD::width_u32());
    static constexpr std::size_t kRegisterCount = RegisterCount;
    static constexpr std::size_t kUnitCount     = kRegisterUnits * kRegisterCount;

    static_assert(sizeof(char32_t) == sizeof(std::uint32_t));

    using Input = std::span<const char32_t, kUnitCount>;

    struct Result
    {
        std::size_t unitsValidated{};
        Status      status{Status::Valid};
    };

    Utf32SimdTile() = delete;
    ~Utf32SimdTile() = delete;

    Utf32SimdTile(const Utf32SimdTile &) = delete;
    Utf32SimdTile &operator=(const Utf32SimdTile &) = delete;
    Utf32SimdTile(Utf32SimdTile &&) = delete;
    Utf32SimdTile &operator=(Utf32SimdTile &&) = delete;

    [[nodiscard]] static inline Result validate(Input input) noexcept
    {
        if (!validateRegister(input.data()))
            return {0, Status::Invalid};

        if constexpr (kRegisterCount == 1) {
            return {kRegisterUnits, Status::Valid};
        } else {
            if (!validateRegister(input.data() + kRegisterUnits))
                return {kRegisterUnits, Status::Invalid};
        }

        if constexpr (kRegisterCount == 2) {
            return {kRegisterUnits * 2, Status::Valid};
        } else {
            if (!validateRegister(input.data() + kRegisterUnits * 2))
                return {kRegisterUnits * 2, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 3))
                return {kRegisterUnits * 3, Status::Invalid};
        }

        if constexpr (kRegisterCount == 4) {
            return {kRegisterUnits * 4, Status::Valid};
        } else {
            if (!validateRegister(input.data() + kRegisterUnits * 4))
                return {kRegisterUnits * 4, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 5))
                return {kRegisterUnits * 5, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 6))
                return {kRegisterUnits * 6, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 7))
                return {kRegisterUnits * 7, Status::Invalid};
        }

        if constexpr (kRegisterCount == 8) {
            return {kRegisterUnits * 8, Status::Valid};
        } else {
            if (!validateRegister(input.data() + kRegisterUnits * 8))
                return {kRegisterUnits * 8, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 9))
                return {kRegisterUnits * 9, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 10))
                return {kRegisterUnits * 10, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 11))
                return {kRegisterUnits * 11, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 12))
                return {kRegisterUnits * 12, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 13))
                return {kRegisterUnits * 13, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 14))
                return {kRegisterUnits * 14, Status::Invalid};

            if (!validateRegister(input.data() + kRegisterUnits * 15))
                return {kRegisterUnits * 15, Status::Invalid};

            return {kUnitCount, Status::Valid};
        }
    }

private:
    [[nodiscard]] static inline bool validateRegister(const char32_t *input) noexcept
    {
        const simd::u32 values              = simd::SIMD::pull_u32(reinterpret_cast<const std::uint32_t *>(input));
        const simd::u32 aboveUnicodeMaximum = simd::SIMD::gt_u32(values, simd::SIMD::set1_u32(0x0010FFFFu));
        const simd::u32 surrogateClass      = simd::SIMD::and_u32(values, simd::SIMD::set1_u32(0xFFFFF800u));
        const simd::u32 surrogates          = simd::SIMD::eq_u32(surrogateClass, simd::SIMD::set1_u32(0x0000D800u));

        // const simd::u32 invalid = simd::SIMD::or_u32(aboveUnicodeMaximum, surrogates);
        return !simd::SIMD::any_u32(simd::SIMD::or_u32(aboveUnicodeMaximum, surrogates));
    }
};

static_assert(Utf32SimdTile<1>::kRegisterUnits > 0);
static_assert(Utf32SimdTile<1>::kUnitCount == Utf32SimdTile<1>::kRegisterUnits);
static_assert(Utf32SimdTile<2>::kUnitCount == Utf32SimdTile<2>::kRegisterUnits * 2);
static_assert(Utf32SimdTile<4>::kUnitCount == Utf32SimdTile<4>::kRegisterUnits * 4);
static_assert(Utf32SimdTile<8>::kUnitCount == Utf32SimdTile<8>::kRegisterUnits * 8);
static_assert(Utf32SimdTile<16>::kUnitCount == Utf32SimdTile<16>::kRegisterUnits * 16);

} // namespace job::core::text