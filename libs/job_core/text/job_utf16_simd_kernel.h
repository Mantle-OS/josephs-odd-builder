#pragma once

#include <cstddef>
#include <cstdint>

#include "job_lattice_codec.h"

#include "job_utf16_kernel.h"
#include "job_utf16_simd_tile.h"

namespace job::core::text {

class Utf16SimdKernel
{
public:
    using Tile8  = Utf16SimdTile<8>;
    using Tile4  = Utf16SimdTile<4>;
    using Tile2  = Utf16SimdTile<2>;
    using Tile1  = Utf16SimdTile<1>;

    static constexpr std::size_t kMinimumInputUnits = Tile1::kUnitCount;

    using State = Utf16SimdState;
    using Result = Utf16Kernel::Result;
    using UnitInput = Utf16Kernel::UnitInput;
    using UnitOutput = Utf16Kernel::UnitOutput;
    using ScalarInput = Utf16Kernel::ScalarInput;
    using ScalarOutput = Utf16Kernel::ScalarOutput;

    Utf16SimdKernel() = delete;
    ~Utf16SimdKernel() = delete;

    Utf16SimdKernel(const Utf16SimdKernel &) = delete;
    Utf16SimdKernel &operator=(const Utf16SimdKernel &) = delete;
    Utf16SimdKernel(Utf16SimdKernel &&) = delete;
    Utf16SimdKernel &operator=(Utf16SimdKernel &&) = delete;

    [[nodiscard]] static inline Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        return Utf16Kernel::decode(input, output);
    }

    [[nodiscard]] static inline Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        return Utf16Kernel::encode(input, output);
    }

    [[nodiscard]] static inline Result validate(UnitInput input) noexcept
    {
        std::size_t inputConsumed{};
        State state{};

        while ((input.size() - inputConsumed) >= Tile8::kUnitCount) {
            if (!consumeTile<Tile8>(input, inputConsumed, state))
                return validateRemainder(input, scalarRestart(inputConsumed, state));
        }

        if ((input.size() - inputConsumed) >= Tile4::kUnitCount) {
            if (!consumeTile<Tile4>(input, inputConsumed, state))
                return validateRemainder(input, scalarRestart(inputConsumed, state));
        }

        if ((input.size() - inputConsumed) >= Tile2::kUnitCount) {
            if (!consumeTile<Tile2>(input, inputConsumed, state))
                return validateRemainder(input, scalarRestart(inputConsumed, state));
        }

        if ((input.size() - inputConsumed) >= Tile1::kUnitCount) {
            if (!consumeTile<Tile1>(input, inputConsumed, state))
                return validateRemainder(input, scalarRestart(inputConsumed, state));
        }

        return validateRemainder(input, scalarRestart(inputConsumed, state));
    }

private:
    template <typename Tile>
    [[nodiscard]] static inline bool consumeTile(UnitInput input, std::size_t &inputConsumed, State &state) noexcept
    {
        const typename Tile::Input tile{input.data() + inputConsumed, Tile::kUnitCount};
        const auto result = Tile::validate(tile, state);

        if (result.status == Utf16SimdTileStatus::Invalid)
            return false;

        inputConsumed += Tile::kUnitCount;
        state = result.state;

        return true;
    }

    [[nodiscard]] static inline std::size_t scalarRestart(std::size_t inputConsumed, const State &state) noexcept
    {
        if (state.empty())
            return inputConsumed;

        return inputConsumed - 1;
    }

    [[nodiscard]] static inline Result validateRemainder(UnitInput input, std::size_t inputConsumed) noexcept
    {
        if (inputConsumed == input.size())
            return {inputConsumed, 0, Utf16Status::Complete};

        const Result remainder = Utf16Kernel::validate(input.subspan(inputConsumed));

        return {
            inputConsumed + remainder.inputConsumed,
            remainder.outputProduced,
            remainder.status
        };
    }
};

static_assert(lattice::LatticeCodec<Utf16SimdKernel>);

} // namespace job::core::text