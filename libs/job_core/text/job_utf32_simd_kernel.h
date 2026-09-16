#pragma once

#include <cstddef>
#include <cstdint>

#include "job_lattice_codec.h"

#include "job_utf32_kernel.h"
#include "job_utf32_simd_tile.h"

namespace job::core::text {

class Utf32SimdKernel
{
public:
    using Tile16 = Utf32SimdTile<16>;
    using Tile8  = Utf32SimdTile<8>;
    using Tile4  = Utf32SimdTile<4>;
    using Tile2  = Utf32SimdTile<2>;
    using Tile1  = Utf32SimdTile<1>;

    static constexpr std::size_t kMinimumInputUnits = Tile1::kUnitCount;

    using Result = Utf32Kernel::Result;
    using UnitInput = Utf32Kernel::UnitInput;
    using UnitOutput = Utf32Kernel::UnitOutput;
    using ScalarInput = Utf32Kernel::ScalarInput;
    using ScalarOutput = Utf32Kernel::ScalarOutput;

    Utf32SimdKernel() = delete;
    ~Utf32SimdKernel() = delete;

    Utf32SimdKernel(const Utf32SimdKernel &) = delete;
    Utf32SimdKernel &operator=(const Utf32SimdKernel &) = delete;
    Utf32SimdKernel(Utf32SimdKernel &&) = delete;
    Utf32SimdKernel &operator=(Utf32SimdKernel &&) = delete;

    [[nodiscard]] static inline Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        return Utf32Kernel::decode(input, output);
    }

    [[nodiscard]] static inline Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        return Utf32Kernel::encode(input, output);
    }

    [[nodiscard]] static inline Result validate(UnitInput input) noexcept
    {
        std::size_t inputConsumed{};

        while ((input.size() - inputConsumed) >= Tile16::kUnitCount) {
            if (!consumeTile<Tile16>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        if ((input.size() - inputConsumed) >= Tile8::kUnitCount) {
            if (!consumeTile<Tile8>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        if ((input.size() - inputConsumed) >= Tile4::kUnitCount) {
            if (!consumeTile<Tile4>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        if ((input.size() - inputConsumed) >= Tile2::kUnitCount) {
            if (!consumeTile<Tile2>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        if ((input.size() - inputConsumed) >= Tile1::kUnitCount) {
            if (!consumeTile<Tile1>(input, inputConsumed))
                return validateRemainder(input, inputConsumed);
        }

        return validateRemainder(input, inputConsumed);
    }

private:
    template <typename Tile>
    [[nodiscard]] static inline bool consumeTile(UnitInput input, std::size_t &inputConsumed) noexcept
    {
        const typename Tile::Input tile{input.data() + inputConsumed, Tile::kUnitCount};
        const auto result = Tile::validate(tile);
        inputConsumed += result.unitsValidated;
        if (result.status == Utf32SimdTileStatus::Invalid)
            return false;

        return true;
    }

    [[nodiscard]] static inline Result validateRemainder(UnitInput input, std::size_t inputConsumed) noexcept
    {
        if (inputConsumed == input.size())
            return {inputConsumed, 0, Utf32Status::Complete};

        const Result remainder = Utf32Kernel::validate(input.subspan(inputConsumed));
        return {
            inputConsumed + remainder.inputConsumed,
            remainder.outputProduced,
            remainder.status
        };
    }
};

static_assert(lattice::LatticeCodec<Utf32SimdKernel>);

} // namespace job::core::text