#pragma once

#include <cstddef>
#include <cstdint>

#include "job_lattice_kernel.h"

#include "job_unicode_scalar.h"
#include "job_utf8_status.h"

namespace job::core::text {

class Utf8Kernel
{
public:
    using Result       = lattice::LatticeKernelResult<Utf8Status>;
    using UnitInput    = lattice::LatticeKernelInput<std::uint8_t>;
    using UnitOutput   = lattice::LatticeKernelOutput<std::uint8_t>;
    using ScalarInput  = lattice::LatticeKernelInput<UnicodeScalar>;
    using ScalarOutput = lattice::LatticeKernelOutput<UnicodeScalar>;

    Utf8Kernel() = delete;
    ~Utf8Kernel() = delete;

    Utf8Kernel(const Utf8Kernel &) = delete;
    Utf8Kernel &operator=(const Utf8Kernel &) = delete;
    Utf8Kernel(Utf8Kernel &&) = delete;
    Utf8Kernel &operator=(Utf8Kernel &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        std::size_t inputConsumed{};
        std::size_t outputProduced{};

        while (inputConsumed < input.size()) {
            if (outputProduced >= output.size())
                return {inputConsumed, outputProduced, Utf8Status::OutputFull};

            const std::uint8_t first = input[inputConsumed];

            if (first <= 0x7F) {
                output[outputProduced] = UnicodeScalar(static_cast<char32_t>(first), UnicodeScalar::unchecked);
                ++inputConsumed;
                ++outputProduced;
                continue;
            }

            if (first >= 0xC2 && first <= 0xDF) {
                if (input.size() - inputConsumed < 2)
                    return {inputConsumed, outputProduced, Utf8Status::Incomplete};

                const std::uint8_t second = input[inputConsumed + 1];

                if (!isContinuation(second))
                    return {inputConsumed, outputProduced, Utf8Status::Invalid};

                const char32_t value = (static_cast<char32_t>(first & 0x1F) << 6) | static_cast<char32_t>(second & 0x3F);

                output[outputProduced] = UnicodeScalar(value, UnicodeScalar::unchecked);
                inputConsumed += 2;
                ++outputProduced;
                continue;
            }

            if (first >= 0xE0 && first <= 0xEF) {
                if (input.size() - inputConsumed < 3)
                    return {inputConsumed, outputProduced, Utf8Status::Incomplete};

                const std::uint8_t second = input[inputConsumed + 1];
                const std::uint8_t third = input[inputConsumed + 2];

                if (!isValidThreeByteSecond(first, second) || !isContinuation(third))
                    return {inputConsumed, outputProduced, Utf8Status::Invalid};

                const char32_t value =
                    (static_cast<char32_t>(first & 0x0F) << 12) |
                    (static_cast<char32_t>(second & 0x3F) << 6) |
                    static_cast<char32_t>(third & 0x3F);

                output[outputProduced] = UnicodeScalar(value, UnicodeScalar::unchecked);
                inputConsumed += 3;
                ++outputProduced;
                continue;
            }

            if (first >= 0xF0 && first <= 0xF4) {
                if (input.size() - inputConsumed < 4)
                    return {inputConsumed, outputProduced, Utf8Status::Incomplete};

                const std::uint8_t second = input[inputConsumed + 1];
                const std::uint8_t third = input[inputConsumed + 2];
                const std::uint8_t fourth = input[inputConsumed + 3];

                if (!isValidFourByteSecond(first, second) ||
                    !isContinuation(third) ||
                    !isContinuation(fourth)) {
                    return {inputConsumed, outputProduced, Utf8Status::Invalid};
                }

                const char32_t value =
                    (static_cast<char32_t>(first & 0x07) << 18) |
                    (static_cast<char32_t>(second & 0x3F) << 12) |
                    (static_cast<char32_t>(third & 0x3F) << 6) |
                    static_cast<char32_t>(fourth & 0x3F);

                output[outputProduced] = UnicodeScalar(value, UnicodeScalar::unchecked);
                inputConsumed += 4;
                ++outputProduced;
                continue;
            }

            return {inputConsumed, outputProduced, Utf8Status::Invalid};
        }

        return {inputConsumed, outputProduced, Utf8Status::Complete};
    }

    [[nodiscard]] static constexpr Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        std::size_t inputConsumed{};
        std::size_t outputProduced{};

        while (inputConsumed < input.size()) {
            const char32_t value = input[inputConsumed].value();

            if (value <= 0x7F) {
                if (output.size() - outputProduced < 1)
                    return {inputConsumed, outputProduced, Utf8Status::OutputFull};

                output[outputProduced] = static_cast<std::uint8_t>(value);
                ++outputProduced;
            } else if (value <= 0x7FF) {
                if (output.size() - outputProduced < 2)
                    return {inputConsumed, outputProduced, Utf8Status::OutputFull};

                output[outputProduced] = static_cast<std::uint8_t>(0xC0 | (value >> 6));
                output[outputProduced + 1] = static_cast<std::uint8_t>(0x80 | (value & 0x3F));
                outputProduced += 2;
            } else if (value <= 0xFFFF) {
                if (output.size() - outputProduced < 3)
                    return {inputConsumed, outputProduced, Utf8Status::OutputFull};

                output[outputProduced] = static_cast<std::uint8_t>(0xE0 | (value >> 12));
                output[outputProduced + 1] = static_cast<std::uint8_t>(0x80 | ((value >> 6) & 0x3F));
                output[outputProduced + 2] = static_cast<std::uint8_t>(0x80 | (value & 0x3F));
                outputProduced += 3;
            } else {
                if (output.size() - outputProduced < 4)
                    return {inputConsumed, outputProduced, Utf8Status::OutputFull};

                output[outputProduced] = static_cast<std::uint8_t>(0xF0 | (value >> 18));
                output[outputProduced + 1] = static_cast<std::uint8_t>(0x80 | ((value >> 12) & 0x3F));
                output[outputProduced + 2] = static_cast<std::uint8_t>(0x80 | ((value >> 6) & 0x3F));
                output[outputProduced + 3] = static_cast<std::uint8_t>(0x80 | (value & 0x3F));
                outputProduced += 4;
            }

            ++inputConsumed;
        }

        return {inputConsumed, outputProduced, Utf8Status::Complete};
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        std::size_t inputConsumed{};

        while (inputConsumed < input.size()) {
            const std::uint8_t first = input[inputConsumed];

            if (first <= 0x7F) {
                ++inputConsumed;
                continue;
            }

            if (first >= 0xC2 && first <= 0xDF) {
                if (input.size() - inputConsumed < 2)
                    return {inputConsumed, 0, Utf8Status::Incomplete};

                if (!isContinuation(input[inputConsumed + 1]))
                    return {inputConsumed, 0, Utf8Status::Invalid};

                inputConsumed += 2;
                continue;
            }

            if (first >= 0xE0 && first <= 0xEF) {
                if (input.size() - inputConsumed < 3)
                    return {inputConsumed, 0, Utf8Status::Incomplete};

                if (!isValidThreeByteSecond(first, input[inputConsumed + 1]) ||
                    !isContinuation(input[inputConsumed + 2])) {
                    return {inputConsumed, 0, Utf8Status::Invalid};
                }

                inputConsumed += 3;
                continue;
            }

            if (first >= 0xF0 && first <= 0xF4) {
                if (input.size() - inputConsumed < 4)
                    return {inputConsumed, 0, Utf8Status::Incomplete};

                if (!isValidFourByteSecond(first, input[inputConsumed + 1]) || !isContinuation(input[inputConsumed + 2]) || !isContinuation(input[inputConsumed + 3]))
                    return {inputConsumed, 0, Utf8Status::Invalid};

                inputConsumed += 4;
                continue;
            }

            return {inputConsumed, 0, Utf8Status::Invalid};
        }

        return {inputConsumed, 0, Utf8Status::Complete};
    }

private:
    [[nodiscard]] static constexpr bool isContinuation(std::uint8_t value) noexcept
    {
        return (value & 0xC0) == 0x80;
    }

    [[nodiscard]] static constexpr bool isValidThreeByteSecond(std::uint8_t first, std::uint8_t second) noexcept
    {
        if (!isContinuation(second))
            return false;

        if (first == 0xE0)
            return second >= 0xA0;

        if (first == 0xED)
            return second <= 0x9F;

        return true;
    }

    [[nodiscard]] static constexpr bool isValidFourByteSecond(std::uint8_t first, std::uint8_t second) noexcept
    {
        if (!isContinuation(second))
            return false;

        if (first == 0xF0)
            return second >= 0x90;

        if (first == 0xF4)
            return second <= 0x8F;

        return true;
    }
};

} // namespace job::core::text
