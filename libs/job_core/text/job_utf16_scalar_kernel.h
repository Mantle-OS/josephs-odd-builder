#pragma once

#include <cstddef>

#include "job_lattice_kernel.h"
#include "job_unicode_scalar.h"
#include "job_utf16_status.h"

namespace job::core::text {

class Utf16ScalarKernel
{
public:
    using Result = lattice::LatticeKernelResult<Utf16Status>;
    using UnitInput = lattice::LatticeKernelInput<char16_t>;
    using UnitOutput = lattice::LatticeKernelOutput<char16_t>;

    Utf16ScalarKernel() = delete;
    ~Utf16ScalarKernel() = delete;

    Utf16ScalarKernel(const Utf16ScalarKernel &) = delete;
    Utf16ScalarKernel &operator=(const Utf16ScalarKernel &) = delete;
    Utf16ScalarKernel(Utf16ScalarKernel &&) = delete;
    Utf16ScalarKernel &operator=(Utf16ScalarKernel &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, UnicodeScalar &output) noexcept
    {
        if (input.empty())
            return {0, 0, Utf16Status::Incomplete};

        const char16_t first = input[0];

        if (!isSurrogate(first)) {
            const auto scalar = UnicodeScalar::from(static_cast<char32_t>(first));

            if (!scalar.has_value())
                return {0, 0, Utf16Status::Invalid};

            output = *scalar;
            return {1, 1, Utf16Status::Complete};
        }

        if (isLowSurrogate(first))
            return {0, 0, Utf16Status::Invalid};

        if (input.size() < 2)
            return {0, 0, Utf16Status::Incomplete};

        const char16_t second = input[1];

        if (!isLowSurrogate(second))
            return {0, 0, Utf16Status::Invalid};

        const auto scalar = UnicodeScalar::from(decodeSurrogatePair(first, second));

        if (!scalar.has_value())
            return {0, 0, Utf16Status::Invalid};

        output = *scalar;
        return {2, 1, Utf16Status::Complete};
    }

    [[nodiscard]] static constexpr Result encode(UnicodeScalar input, UnitOutput output) noexcept
    {
        const char32_t value = input.value();

        if (value <= 0xFFFF) {
            if (output.empty())
                return {0, 0, Utf16Status::OutputFull};

            output[0] = static_cast<char16_t>(value);
            return {1, 1, Utf16Status::Complete};
        }

        if (output.size() < 2)
            return {0, 0, Utf16Status::OutputFull};

        const char32_t adjusted = value - 0x10000;

        output[0] = static_cast<char16_t>(0xD800 + (adjusted >> 10));
        output[1] = static_cast<char16_t>(0xDC00 + (adjusted & 0x03FF));

        return {1, 2, Utf16Status::Complete};
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        UnicodeScalar scalar{};
        return decode(input, scalar);
    }

private:
    [[nodiscard]] static constexpr char32_t decodeSurrogatePair(char16_t high, char16_t low) noexcept
        pre(isHighSurrogate(high))
        pre(isLowSurrogate(low))
    {
        const char32_t highValue = static_cast<char32_t>(high) - 0xD800;
        const char32_t lowValue = static_cast<char32_t>(low) - 0xDC00;

        return 0x10000 + ((highValue << 10) | lowValue);
    }

    [[nodiscard]] static constexpr bool isSurrogate(char16_t value) noexcept
    {
        return value >= 0xD800 && value <= 0xDFFF;
    }

    [[nodiscard]] static constexpr bool isHighSurrogate(char16_t value) noexcept
    {
        return value >= 0xD800 && value <= 0xDBFF;
    }

    [[nodiscard]] static constexpr bool isLowSurrogate(char16_t value) noexcept
    {
        return value >= 0xDC00 && value <= 0xDFFF;
    }


};

} // namespace job::core::text