#pragma once

#include <cstddef>
#include <cstdint>

#include "job_ascii_simd_tile.h"
#include "job_utf8_kernel.h"

namespace job::core::text {

class AsciiSimdKernel
{
public:
    using Tile4 = AsciiSimdTile<4>;
    using Tile2 = AsciiSimdTile<2>;
    using Tile1 = AsciiSimdTile<1>;

    static constexpr std::size_t kMinimumInputBytes = Tile1::kByteCount;

    using Result = Utf8Kernel::Result;

    using UnitInput = Utf8Kernel::UnitInput;
    using UnitOutput = Utf8Kernel::UnitOutput;
    using ScalarInput = Utf8Kernel::ScalarInput;
    using ScalarOutput = Utf8Kernel::ScalarOutput;

    AsciiSimdKernel() = delete;
    ~AsciiSimdKernel() = delete;

    AsciiSimdKernel(const AsciiSimdKernel &) = delete;
    AsciiSimdKernel &operator=(const AsciiSimdKernel &) = delete;
    AsciiSimdKernel(AsciiSimdKernel &&) = delete;
    AsciiSimdKernel &operator=(AsciiSimdKernel &&) = delete;

    [[nodiscard]] static inline Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        return Utf8Kernel::decode(input, output);
    }

    [[nodiscard]] static inline Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        return Utf8Kernel::encode(input, output);
    }

    [[nodiscard]] static inline Result validate(UnitInput input) noexcept
    {
        std::size_t inputConsumed{};

        while (input.size() - inputConsumed >= Tile4::kByteCount) {
            if (!consumeTile<Tile4>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        if (input.size() - inputConsumed >= Tile2::kByteCount) {
            if (!consumeTile<Tile2>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        if (input.size() - inputConsumed >= Tile1::kByteCount) {
            if (!consumeTile<Tile1>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        return validateRemainder(input, inputConsumed);
    }

private:
    template <typename Tile>
    [[nodiscard]] static inline bool consumeTile(UnitInput input, std::size_t &inputConsumed) noexcept
    {
        const typename Tile::Input tile{
            input.data() + inputConsumed,
            Tile::kByteCount
        };

        const std::size_t asciiBytes = Tile::asciiPrefixBytes(tile);
        inputConsumed += asciiBytes;

        return asciiBytes == Tile::kByteCount;
    }

    [[nodiscard]] static inline Result validateRemainder(UnitInput input, std::size_t inputConsumed) noexcept
    {
        if (inputConsumed == input.size())
            return {inputConsumed, 0, Utf8Status::Complete};

        const Result result = Utf8Kernel::validate(input.subspan(inputConsumed));

        return {
            inputConsumed + result.inputConsumed,
            result.outputProduced,
            result.status
        };
    }
};

} // namespace job::core::text