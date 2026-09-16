#pragma once

#include <cstddef>

#include "job_lattice_codec.h"
#include "job_lattice_kernel.h"

#include "job_unicode_scalar.h"
#include "job_utf16_status.h"

namespace job::core::text {

class Utf16Kernel
{
public:
    using Result       = lattice::LatticeKernelResult<Utf16Status>;

    using UnitInput    = lattice::LatticeKernelInput<char16_t>;
    using UnitOutput   = lattice::LatticeKernelOutput<char16_t>;

    using ScalarInput  = lattice::LatticeKernelInput<UnicodeScalar>;
    using ScalarOutput = lattice::LatticeKernelOutput<UnicodeScalar>;

    Utf16Kernel() = delete;
    ~Utf16Kernel() = delete;

    Utf16Kernel(const Utf16Kernel &) = delete;
    Utf16Kernel &operator=(const Utf16Kernel &) = delete;
    Utf16Kernel(Utf16Kernel &&) = delete;
    Utf16Kernel &operator=(Utf16Kernel &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        std::size_t inputConsumed{};
        std::size_t outputProduced{};

        while (inputConsumed < input.size()) {
            if (outputProduced >= output.size())
                return {inputConsumed, outputProduced, Utf16Status::OutputFull};

            const char16_t first = input[inputConsumed];

            if (!isSurrogate(first)) {
                output[outputProduced] =
                    UnicodeScalar(static_cast<char32_t>(first), UnicodeScalar::unchecked);

                ++inputConsumed;
                ++outputProduced;
                continue;
            }

            if (isLowSurrogate(first))
                return {inputConsumed, outputProduced, Utf16Status::Invalid};

            if (input.size() - inputConsumed < 2)
                return {inputConsumed, outputProduced, Utf16Status::Incomplete};

            const char16_t second = input[inputConsumed + 1];

            if (!isLowSurrogate(second))
                return {inputConsumed, outputProduced, Utf16Status::Invalid};

            const char32_t high = static_cast<char32_t>(first) - 0xD800;
            const char32_t low = static_cast<char32_t>(second) - 0xDC00;
            const char32_t value = 0x10000 + ((high << 10) | low);

            output[outputProduced] = UnicodeScalar(value, UnicodeScalar::unchecked);

            inputConsumed += 2;
            ++outputProduced;
        }

        return {inputConsumed, outputProduced, Utf16Status::Complete};
    }

    [[nodiscard]] static constexpr Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        std::size_t inputConsumed{};
        std::size_t outputProduced{};

        while (inputConsumed < input.size()) {
            const char32_t value = input[inputConsumed].value();

            if (value <= 0xFFFF) {
                if (outputProduced >= output.size())
                    return {inputConsumed, outputProduced, Utf16Status::OutputFull};

                output[outputProduced] = static_cast<char16_t>(value);

                ++inputConsumed;
                ++outputProduced;
                continue;
            }

            if (output.size() - outputProduced < 2)
                return {inputConsumed, outputProduced, Utf16Status::OutputFull};

            const char32_t adjusted = value - 0x10000;

            output[outputProduced] =
                static_cast<char16_t>(0xD800 + (adjusted >> 10));

            output[outputProduced + 1] =
                static_cast<char16_t>(0xDC00 + (adjusted & 0x3FF));

            ++inputConsumed;
            outputProduced += 2;
        }

        return {inputConsumed, outputProduced, Utf16Status::Complete};
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        std::size_t inputConsumed{};

        while (inputConsumed < input.size()) {
            const char16_t first = input[inputConsumed];

            if (!isSurrogate(first)) {
                ++inputConsumed;
                continue;
            }

            if (isLowSurrogate(first))
                return {inputConsumed, 0, Utf16Status::Invalid};

            if (input.size() - inputConsumed < 2)
                return {inputConsumed, 0, Utf16Status::Incomplete};

            if (!isLowSurrogate(input[inputConsumed + 1]))
                return {inputConsumed, 0, Utf16Status::Invalid};

            inputConsumed += 2;
        }

        return {inputConsumed, 0, Utf16Status::Complete};
    }

private:
    [[nodiscard]] static constexpr bool isHighSurrogate(char16_t value) noexcept
    {
        return value >= 0xD800 && value <= 0xDBFF;
    }

    [[nodiscard]] static constexpr bool isLowSurrogate(char16_t value) noexcept
    {
        return value >= 0xDC00 && value <= 0xDFFF;
    }

    [[nodiscard]] static constexpr bool isSurrogate(char16_t value) noexcept
    {
        return value >= 0xD800 && value <= 0xDFFF;
    }
};

} // namespace job::core::text



// #pragma once

// #include <cstddef>

// #include "job_lattice_kernel.h"
// #include "job_lattice_codec.h"

// #include "job_unicode_scalar.h"
// #include "job_utf16_scalar_kernel.h"
// #include "job_utf16_status.h"

// namespace job::core::text {

// class Utf16Kernel
// {
// public:
//     using Result       = lattice::LatticeKernelResult<Utf16Status>;

//     using UnitInput    = lattice::LatticeKernelInput<char16_t>;
//     using UnitOutput   = lattice::LatticeKernelOutput<char16_t>;

//     using ScalarInput  = lattice::LatticeKernelInput<UnicodeScalar>;
//     using ScalarOutput = lattice::LatticeKernelOutput<UnicodeScalar>;

//     Utf16Kernel() = delete;
//     ~Utf16Kernel() = delete;

//     Utf16Kernel(const Utf16Kernel &) = delete;
//     Utf16Kernel &operator=(const Utf16Kernel &) = delete;
//     Utf16Kernel(Utf16Kernel &&) = delete;
//     Utf16Kernel &operator=(Utf16Kernel &&) = delete;

//     [[nodiscard]] static constexpr Result decode(UnitInput input, ScalarOutput output) noexcept
//     {
//         std::size_t inputConsumed  = 0;
//         std::size_t outputProduced = 0;

//         while (inputConsumed < input.size()) {
//             if (outputProduced >= output.size())
//                 return {inputConsumed, outputProduced, Utf16Status::OutputFull};

//             const auto result = Utf16ScalarKernel::decode(input.subspan(inputConsumed), output[outputProduced]);
//             if (result.status != Utf16Status::Complete)
//                 return {inputConsumed, outputProduced, result.status};

//             inputConsumed  += result.inputConsumed;
//             outputProduced += result.outputProduced;
//         }

//         return { inputConsumed, outputProduced, Utf16Status::Complete };
//     }

//     [[nodiscard]] static constexpr Result encode(ScalarInput input, UnitOutput output) noexcept
//     {
//         std::size_t inputConsumed  = 0;
//         std::size_t outputProduced = 0;
//         while (inputConsumed < input.size()) {
//             const auto result = Utf16ScalarKernel::encode(input[inputConsumed], output.subspan(outputProduced));
//             if (result.status != Utf16Status::Complete)
//                 return {inputConsumed, outputProduced, result.status};

//             inputConsumed  += result.inputConsumed;
//             outputProduced += result.outputProduced;
//         }

//         return { inputConsumed, outputProduced, Utf16Status::Complete };
//     }

//     [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
//     {
//         std::size_t inputConsumed = 0;
//         while (inputConsumed < input.size()) {
//             const auto result = Utf16ScalarKernel::validate(input.subspan(inputConsumed));
//             if (result.status != Utf16Status::Complete)
//                 return {inputConsumed, 0, result.status};

//             inputConsumed += result.inputConsumed;
//         }

//         return { inputConsumed, 0, Utf16Status::Complete };
//     }
// };

// } // namespace job::core::text