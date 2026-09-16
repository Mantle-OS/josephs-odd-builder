#pragma once

#include <algorithm>
#include <cstddef>

#include "job_lattice_kernel.h"

#include "job_unicode_scalar.h"
#include "job_utf32_status.h"

namespace job::core::text {

class Utf32Kernel
{
public:
    using Result       = lattice::LatticeKernelResult<Utf32Status>;
    using UnitInput    = lattice::LatticeKernelInput<char32_t>;
    using UnitOutput   = lattice::LatticeKernelOutput<char32_t>;
    using ScalarInput  = lattice::LatticeKernelInput<UnicodeScalar>;
    using ScalarOutput = lattice::LatticeKernelOutput<UnicodeScalar>;

    Utf32Kernel() = delete;
    ~Utf32Kernel() = delete;

    Utf32Kernel(const Utf32Kernel &) = delete;
    Utf32Kernel &operator=(const Utf32Kernel &) = delete;
    Utf32Kernel(Utf32Kernel &&) = delete;
    Utf32Kernel &operator=(Utf32Kernel &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        const std::size_t count = std::min(input.size(), output.size());

        for (std::size_t i = 0; i < count; ++i) {
            const char32_t value = input[i];

            if (!UnicodeScalar::isValid(value))
                return {i, i, Utf32Status::Invalid};

            output[i] = UnicodeScalar(value, UnicodeScalar::unchecked);
        }

        if (count < input.size())
            return {count, count, Utf32Status::OutputFull};

        return {count, count, Utf32Status::Complete};
    }

    [[nodiscard]] static constexpr Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        const std::size_t count = std::min(input.size(), output.size());

        for (std::size_t i = 0; i < count; ++i)
            output[i] = input[i].value();

        if (count < input.size())
            return {count, count, Utf32Status::OutputFull};

        return {count, count, Utf32Status::Complete};
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        for (std::size_t i = 0; i < input.size(); ++i) {
            if (!UnicodeScalar::isValid(input[i]))
                return {i, 0, Utf32Status::Invalid};
        }

        return {input.size(), 0, Utf32Status::Complete};
    }
};

} // namespace job::core::text