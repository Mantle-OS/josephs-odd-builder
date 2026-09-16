#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "simd_provider.h"

namespace job::core::text {

struct Utf16SimdState
{
    bool expectsLowSurrogate{};

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return !expectsLowSurrogate;
    }
};

enum class Utf16SimdTileStatus : std::uint8_t
{
    Valid,
    Invalid
};

template <std::size_t RegisterCount>
class Utf16SimdTile
{
public:
    static_assert(RegisterCount == 1 || RegisterCount == 2 || RegisterCount == 4 || RegisterCount == 8,
                  "Utf16SimdTile supports 1, 2, 4, or 8 SIMD registers");

    using State  = Utf16SimdState;
    using Status = Utf16SimdTileStatus;

    static constexpr std::size_t kRegisterUnits = static_cast<std::size_t>(simd::SIMD::width_u16());
    static constexpr std::size_t kRegisterCount = RegisterCount;
    static constexpr std::size_t kUnitCount     = kRegisterUnits * kRegisterCount;

    using Input = std::span<const char16_t, kUnitCount>;
    struct Result
    {
        std::size_t unitsValidated{};
        State       state{};
        Status      status{Status::Valid};
    };

    Utf16SimdTile() = delete;
    ~Utf16SimdTile() = delete;

    Utf16SimdTile(const Utf16SimdTile &) = delete;
    Utf16SimdTile &operator=(const Utf16SimdTile &) = delete;
    Utf16SimdTile(Utf16SimdTile &&) = delete;
    Utf16SimdTile &operator=(Utf16SimdTile &&) = delete;

    [[nodiscard]] static inline Result validate(Input input, State state = {}) noexcept
    {
        RegisterResult result = validateRegister(input.data(), state);

        if (result.status == Status::Invalid)
            return {0, {}, Status::Invalid};

        if constexpr (kRegisterCount == 1) {
            return {kRegisterUnits, result.state, Status::Valid};
        } else {
            result = validateRegister(input.data() + kRegisterUnits, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterUnits, {}, Status::Invalid};
        }

        if constexpr (kRegisterCount == 2) {
            return {kRegisterUnits * 2, result.state, Status::Valid};
        } else {
            result = validateRegister(input.data() + kRegisterUnits * 2, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterUnits * 2, {}, Status::Invalid};

            result = validateRegister(input.data() + kRegisterUnits * 3, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterUnits * 3, {}, Status::Invalid};
        }

        if constexpr (kRegisterCount == 4) {
            return {kRegisterUnits * 4, result.state, Status::Valid};
        } else {
            result = validateRegister(input.data() + kRegisterUnits * 4, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterUnits * 4, {}, Status::Invalid};

            result = validateRegister(input.data() + kRegisterUnits * 5, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterUnits * 5, {}, Status::Invalid};

            result = validateRegister(input.data() + kRegisterUnits * 6, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterUnits * 6, {}, Status::Invalid};

            result = validateRegister(input.data() + kRegisterUnits * 7, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterUnits * 7, {}, Status::Invalid};

            return {kUnitCount, result.state, Status::Valid};
        }
    }

private:
    struct RegisterResult
    {
        State state{};
        Status status{Status::Valid};
    };

    [[nodiscard]] static inline RegisterResult validateRegister(const char16_t *input, State state) noexcept
    {
        const simd::u16 values         = simd::SIMD::pull_u16(reinterpret_cast<const std::uint16_t *>(input));
        const simd::u16 surrogateClass = simd::SIMD::and_u16(values, simd::SIMD::set1_u16(0xFC00));

        const simd::u16 highSurrogates = simd::SIMD::eq_u16(surrogateClass, simd::SIMD::set1_u16(0xD800));
        const simd::u16 lowSurrogates  = simd::SIMD::eq_u16(surrogateClass, simd::SIMD::set1_u16(0xDC00));
        const std::uint32_t highMask   = simd::SIMD::movemask_u16(highSurrogates);
        const std::uint32_t lowMask    = simd::SIMD::movemask_u16(lowSurrogates);

        constexpr std::size_t width = static_cast<std::size_t>(simd::SIMD::width_u16());
        static_assert(width > 0 && width <= 32);

        constexpr std::uint32_t laneMask = [] {
            if constexpr (width == 32){
                return std::uint32_t{0xFFFFFFFFu};

            } else {
                return (std::uint32_t{1} << width) - 1;
            }
        }();

        std::uint32_t expectedLowMask = (highMask << 1) & laneMask;
        if (state.expectsLowSurrogate)
            expectedLowMask |= 1u;

        if (lowMask != expectedLowMask)
            return {{}, Status::Invalid};

        constexpr std::uint32_t lastLaneMask = std::uint32_t{1} << (width - 1);
        State nextState{};
        nextState.expectsLowSurrogate = (highMask & lastLaneMask) != 0;

        return {nextState, Status::Valid};
    }
};

static_assert(Utf16SimdTile<1>::kRegisterUnits > 0);
static_assert(Utf16SimdTile<1>::kUnitCount == Utf16SimdTile<1>::kRegisterUnits);
static_assert(Utf16SimdTile<2>::kUnitCount == Utf16SimdTile<2>::kRegisterUnits * 2);
static_assert(Utf16SimdTile<4>::kUnitCount == Utf16SimdTile<4>::kRegisterUnits * 4);
static_assert(Utf16SimdTile<8>::kUnitCount == Utf16SimdTile<8>::kRegisterUnits * 8);

} // namespace job::core::text