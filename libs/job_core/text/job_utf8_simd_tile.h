#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "simd_provider.h"

namespace job::core::text {
enum class Utf8SimdFirstContinuationRule : std::uint8_t
{
    Any,
    A0_BF,
    Eighty_9F,
    Ninety_BF,
    Eighty_8F
};

struct Utf8SimdState
{
    std::uint8_t expectedContinuationMask{};
    std::uint8_t trailingSequenceBytes{};
    Utf8SimdFirstContinuationRule firstContinuationRule{Utf8SimdFirstContinuationRule::Any};

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return expectedContinuationMask == 0;
    }
};

enum class Utf8SimdTileStatus : std::uint8_t
{
    Valid,
    Invalid
};

template <std::size_t RegisterCount>
class Utf8SimdTile
{
public:
    using State                 = Utf8SimdState;
    using FirstContinuationRule = Utf8SimdFirstContinuationRule;
    using Status                = Utf8SimdTileStatus;

    static constexpr std::size_t kRegisterBytes = static_cast<std::size_t>(simd::SIMD::width_u8());
    static constexpr std::size_t kRegisterCount = RegisterCount;

    static constexpr std::size_t kByteCount     = kRegisterBytes * kRegisterCount;
    using Input                                 = std::span<const std::uint8_t, kByteCount>;

    static_assert(RegisterCount == 1 || RegisterCount == 2 || RegisterCount == 4, "Utf8SimdTile supports 1, 2, or 4 SIMD registers");


    struct Result
    {
        std::size_t bytesValidated{};
        State       state{};
        Status      status{Status::Valid};
    };

    Utf8SimdTile() = delete;
    ~Utf8SimdTile() = delete;

    Utf8SimdTile(const Utf8SimdTile &) = delete;
    Utf8SimdTile &operator=(const Utf8SimdTile &) = delete;
    Utf8SimdTile(Utf8SimdTile &&) = delete;
    Utf8SimdTile &operator=(Utf8SimdTile &&) = delete;

    [[nodiscard]] static inline Result validate(Input input, State state = {}) noexcept
    {
        RegisterResult result = validateRegister(input.data(), state);

        if (result.status == Status::Invalid)
            return {0, {}, Status::Invalid};

        if constexpr (kRegisterCount == 1) {
            return {kRegisterBytes, result.state, Status::Valid};
        } else {
            result = validateRegister(input.data() + kRegisterBytes, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterBytes, {}, Status::Invalid};
        }

        if constexpr (kRegisterCount == 2) {
            return {kRegisterBytes * 2, result.state, Status::Valid};
        } else {
            result = validateRegister(input.data() + kRegisterBytes * 2, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterBytes * 2, {}, Status::Invalid};

            result = validateRegister(input.data() + kRegisterBytes * 3, result.state);

            if (result.status == Status::Invalid)
                return {kRegisterBytes * 3, {}, Status::Invalid};

            return {kByteCount, result.state, Status::Valid};
        }
    }

private:
    enum class ByteClass : std::uint8_t
    {
        Ascii = 1,
        Continuation,
        Lead2,
        Lead3,
        Lead4
    };

    struct RegisterResult
    {
        State state{};
        Status status{Status::Valid};
    };

    alignas(32) inline static constexpr std::array<std::uint8_t, 32> kClassTable{
        // High nibble 0x0..0xF.
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Lead2),
        static_cast<std::uint8_t>(ByteClass::Lead2),
        static_cast<std::uint8_t>(ByteClass::Lead3),
        static_cast<std::uint8_t>(ByteClass::Lead4),

        // AVX2 pshufb operates independently on its two 128-bit lanes.
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Ascii),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Continuation),
        static_cast<std::uint8_t>(ByteClass::Lead2),
        static_cast<std::uint8_t>(ByteClass::Lead2),
        static_cast<std::uint8_t>(ByteClass::Lead3),
        static_cast<std::uint8_t>(ByteClass::Lead4)
    };

    alignas(32) inline static constexpr std::array<std::uint8_t, 32> kInvalidFTable{
        0x00, 0x00, 0x00, 0x00,
        0x00,
        0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,

        0x00, 0x00, 0x00, 0x00,
        0x00,
        0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF
    };

    __attribute__((always_inline))
    [[nodiscard]] static inline RegisterResult validateRegister(const std::uint8_t *input, State state) noexcept
    {
        const simd::u8 bytes = simd::SIMD::pull_u8(input);

        // already-proven ASCII fast path.
        if (state.empty() && !simd::SIMD::any_high_bit_u8(bytes))
            return {};

        const simd::u16 shifted = simd::SIMD::srli_u16<4>(bytes);
        const simd::u8 lowNibble = simd::SIMD::and_u8(bytes, simd::SIMD::set1_u8(0x0F));
        const simd::u8 highNibble = simd::SIMD::and_u8(shifted, simd::SIMD::set1_u8(0x0F));
        const simd::u8 classTable = simd::SIMD::pull_u8(kClassTable.data());
        const simd::u8 classes = simd::SIMD::shuffle_u8(classTable, highNibble);
        const std::uint32_t continuationMask = classMask(classes, ByteClass::Continuation);
        const std::uint32_t lead2Mask = classMask(classes, ByteClass::Lead2);
        const std::uint32_t lead3Mask = classMask(classes, ByteClass::Lead3);
        const std::uint32_t lead4Mask = classMask(classes, ByteClass::Lead4);

        // C0/C1 are overlong lead candidates and therefore invalid.
        const simd::u8 c0 = simd::SIMD::eq_u8(bytes, simd::SIMD::set1_u8(0xC0));
        const simd::u8 c1 = simd::SIMD::eq_u8(bytes, simd::SIMD::set1_u8(0xC1));

        // F5..FF are above the Unicode scalar range.
        const simd::u8 invalidFTable = simd::SIMD::pull_u8(kInvalidFTable.data());
        const simd::u8 invalidFLow = simd::SIMD::shuffle_u8(invalidFTable, lowNibble);
        const simd::u8 lead4Class = simd::SIMD::eq_u8(classes, simd::SIMD::set1_u8(static_cast<std::uint8_t>(ByteClass::Lead4)));
        const simd::u8 invalidF = simd::SIMD::and_u8(invalidFLow, lead4Class);
        const simd::u8 invalidLead = simd::SIMD::or_u8(simd::SIMD::or_u8(c0, c1), invalidF);

        if (simd::SIMD::movemask_u8(invalidLead) != 0)
            return {{}, Status::Invalid};

        const std::uint64_t expectedContinuationMask =
            static_cast<std::uint64_t>(state.expectedContinuationMask) |
            (static_cast<std::uint64_t>(lead2Mask) << 1) |
            (static_cast<std::uint64_t>(lead3Mask) << 1) |
            (static_cast<std::uint64_t>(lead3Mask) << 2) |
            (static_cast<std::uint64_t>(lead4Mask) << 1) |
            (static_cast<std::uint64_t>(lead4Mask) << 2) |
            (static_cast<std::uint64_t>(lead4Mask) << 3);

        if (static_cast<std::uint32_t>(expectedContinuationMask) != continuationMask)
            return {{}, Status::Invalid};

        const std::uint32_t nibble8 = nibbleMask(highNibble, 0x08);
        const std::uint32_t nibble9 = nibbleMask(highNibble, 0x09);
        const std::uint32_t nibbleA = nibbleMask(highNibble, 0x0A);
        const std::uint32_t nibbleB = nibbleMask(highNibble, 0x0B);

        if (!validateIncomingFirstContinuation(state.firstContinuationRule,
                                               nibble8, nibble9,
                                               nibbleA, nibbleB)) {
            return {{}, Status::Invalid};
        }

        const std::uint32_t e0Mask = byteMask(bytes, 0xE0);
        const std::uint32_t edMask = byteMask(bytes, 0xED);
        const std::uint32_t f0Mask = byteMask(bytes, 0xF0);
        const std::uint32_t f4Mask = byteMask(bytes, 0xF4);

        if (!validateSpecialFirstContinuations(e0Mask,  edMask,
                                               f0Mask,  f4Mask,
                                               nibble8, nibble9,
                                               nibbleA, nibbleB)) {
            return {{}, Status::Invalid};
        }

        State nextState{};
        nextState.expectedContinuationMask = static_cast<std::uint8_t>((expectedContinuationMask >> 32) & 0x07);
        nextState.firstContinuationRule = outgoingFirstContinuationRule(e0Mask, edMask, f0Mask, f4Mask);
        nextState.trailingSequenceBytes =trailingSequenceBytes(lead2Mask,
                                                                lead3Mask,
                                                                lead4Mask,
                                                                nextState.expectedContinuationMask);

        return {nextState, Status::Valid};
    }

    __attribute__((always_inline))
    [[nodiscard]] static inline std::uint32_t classMask(simd::u8 classes, ByteClass byteClass) noexcept
    {
        const simd::u8 mask = simd::SIMD::eq_u8(classes,
                                                simd::SIMD::set1_u8(static_cast<std::uint8_t>(byteClass)));

        return simd::SIMD::movemask_u8(mask);
    }

    __attribute__((always_inline))
    [[nodiscard]] static inline std::uint32_t byteMask(simd::u8 bytes, std::uint8_t value) noexcept
    {
        return simd::SIMD::movemask_u8(simd::SIMD::eq_u8(bytes, simd::SIMD::set1_u8(value)));
    }

    __attribute__((always_inline))
    [[nodiscard]] static inline std::uint32_t nibbleMask(simd::u8 highNibble, std::uint8_t value) noexcept
    {
        return simd::SIMD::movemask_u8(
            simd::SIMD::eq_u8(highNibble, simd::SIMD::set1_u8(value)));
    }

    [[nodiscard]] static inline bool validateIncomingFirstContinuation(FirstContinuationRule rule,
                                                                       std::uint32_t nibble8,
                                                                       std::uint32_t nibble9,
                                                                       std::uint32_t nibbleA,
                                                                       std::uint32_t nibbleB) noexcept
    {
        if (rule == FirstContinuationRule::Any)
            return true;

        const std::uint32_t firstByteMask = 1;

        switch (rule) {
        case FirstContinuationRule::A0_BF:
            return ((nibbleA | nibbleB) & firstByteMask) != 0;

        case FirstContinuationRule::Eighty_9F:
            return ((nibble8 | nibble9) & firstByteMask) != 0;

        case FirstContinuationRule::Ninety_BF:
            return ((nibble9 | nibbleA | nibbleB) & firstByteMask) != 0;

        case FirstContinuationRule::Eighty_8F:
            return (nibble8 & firstByteMask) != 0;

        case FirstContinuationRule::Any:
            return true;
        }

        return false;
    }

    [[nodiscard]] static inline bool validateSpecialFirstContinuations(std::uint32_t e0Mask,
                                                                       std::uint32_t edMask,
                                                                       std::uint32_t f0Mask,
                                                                       std::uint32_t f4Mask,
                                                                       std::uint32_t nibble8,
                                                                       std::uint32_t nibble9,
                                                                       std::uint32_t nibbleA,
                                                                       std::uint32_t nibbleB) noexcept
    {
        const std::uint32_t allowE0 = nibbleA | nibbleB;
        const std::uint32_t allowED = nibble8 | nibble9;
        const std::uint32_t allowF0 = nibble9 | nibbleA | nibbleB;
        const std::uint32_t allowF4 = nibble8;

        if (((e0Mask << 1) & ~allowE0) != 0)
            return false;

        if (((edMask << 1) & ~allowED) != 0)
            return false;

        if (((f0Mask << 1) & ~allowF0) != 0)
            return false;

        if (((f4Mask << 1) & ~allowF4) != 0)
            return false;

        return true;
    }

    [[nodiscard]] static inline FirstContinuationRule outgoingFirstContinuationRule(std::uint32_t e0Mask,
                                                                                    std::uint32_t edMask,
                                                                                    std::uint32_t f0Mask,
                                                                                    std::uint32_t f4Mask) noexcept
    {
        constexpr std::uint32_t lastByte = std::uint32_t{1} << 31;

        if ((e0Mask & lastByte) != 0)
            return FirstContinuationRule::A0_BF;

        if ((edMask & lastByte) != 0)
            return FirstContinuationRule::Eighty_9F;

        if ((f0Mask & lastByte) != 0)
            return FirstContinuationRule::Ninety_BF;

        if ((f4Mask & lastByte) != 0)
            return FirstContinuationRule::Eighty_8F;

        return FirstContinuationRule::Any;
    }

    [[nodiscard]] static inline std::uint8_t trailingSequenceBytes(std::uint32_t lead2Mask,
                                                                   std::uint32_t lead3Mask,
                                                                   std::uint32_t lead4Mask,
                                                                   std::uint8_t expectedContinuationMask) noexcept
    {
        if (expectedContinuationMask == 0)
            return 0;

        constexpr std::uint32_t bit31 = std::uint32_t{1} << 31;
        constexpr std::uint32_t bit30 = std::uint32_t{1} << 30;
        constexpr std::uint32_t bit29 = std::uint32_t{1} << 29;

        if ((lead4Mask & bit31) != 0)
            return 1;

        if ((lead4Mask & bit30) != 0)
            return 2;

        if ((lead4Mask & bit29) != 0)
            return 3;

        if ((lead3Mask & bit31) != 0)
            return 1;

        if ((lead3Mask & bit30) != 0)
            return 2;

        if ((lead2Mask & bit31) != 0)
            return 1;

        return 0;
    }
};

static_assert(Utf8SimdTile<1>::kRegisterBytes > 0);
static_assert(Utf8SimdTile<1>::kByteCount == Utf8SimdTile<1>::kRegisterBytes);
static_assert(Utf8SimdTile<2>::kByteCount == Utf8SimdTile<2>::kRegisterBytes * 2);
static_assert(Utf8SimdTile<4>::kByteCount == Utf8SimdTile<4>::kRegisterBytes * 4);

} // namespace job::core::text