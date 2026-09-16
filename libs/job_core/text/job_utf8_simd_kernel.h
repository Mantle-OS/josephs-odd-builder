#pragma once

#include <cstddef>
#include <cstdint>

#include "job_utf8_kernel.h"
#include "job_utf8_simd_tile.h"

namespace job::core::text {

class Utf8SimdKernel
{
public:

    using Tile4         = Utf8SimdTile<4>;
    using Tile2         = Utf8SimdTile<2>;
    using Tile1         = Utf8SimdTile<1>;
    static constexpr std::size_t kMinimumInputBytes = Tile1::kByteCount;

    using State         = Utf8SimdState;
    using Result        = Utf8Kernel::Result;
    using UnitInput     = Utf8Kernel::UnitInput;
    using UnitOutput    = Utf8Kernel::UnitOutput;
    using ScalarInput   = Utf8Kernel::ScalarInput;
    using ScalarOutput  = Utf8Kernel::ScalarOutput;

    Utf8SimdKernel() = delete;
    ~Utf8SimdKernel() = delete;

    Utf8SimdKernel(const Utf8SimdKernel &) = delete;
    Utf8SimdKernel &operator=(const Utf8SimdKernel &) = delete;
    Utf8SimdKernel(Utf8SimdKernel &&) = delete;
    Utf8SimdKernel &operator=(Utf8SimdKernel &&) = delete;

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
        State state{};

        while ((input.size() - inputConsumed) >= Tile4::kByteCount) {
            if (!consumeTile<Tile4>(input, inputConsumed, state))
                return validateRemainder(input, scalarRestart(inputConsumed, state));
        }

        if ((input.size() - inputConsumed) >= Tile2::kByteCount) {
            if (!consumeTile<Tile2>(input, inputConsumed, state))
                return validateRemainder(input, scalarRestart(inputConsumed, state));
        }

        if ((input.size() - inputConsumed) >= Tile1::kByteCount) {
            if (!consumeTile<Tile1>(input, inputConsumed, state))
                return validateRemainder(input, scalarRestart(inputConsumed, state));
        }

        return validateRemainder(input, scalarRestart(inputConsumed, state));
    }

private:
    template <typename Tile>
    [[nodiscard]] static inline bool consumeTile(UnitInput input, std::size_t &inputConsumed, State &state) noexcept
    {
        const typename Tile::Input tile{input.data() + inputConsumed, Tile::kByteCount};
        const auto result = Tile::validate(tile, state);
        if (result.status == Utf8SimdTileStatus::Invalid)
            return false;

        inputConsumed += Tile::kByteCount;
        state = result.state;

        return true;
    }

    [[nodiscard]] static inline std::size_t scalarRestart(std::size_t inputConsumed, const State &state) noexcept
    {
        if (state.empty())
            return inputConsumed;

        return inputConsumed - state.trailingSequenceBytes;
    }

    [[nodiscard]] static inline Result validateRemainder(UnitInput input, std::size_t inputConsumed) noexcept
    {
        if (inputConsumed == input.size())
            return {inputConsumed, 0, Utf8Status::Complete};

        const Result remainder = Utf8Kernel::validate(input.subspan(inputConsumed));
        return {
            inputConsumed + remainder.inputConsumed,
            remainder.outputProduced,
            remainder.status
        };
    }
};

} // namespace job::core::text