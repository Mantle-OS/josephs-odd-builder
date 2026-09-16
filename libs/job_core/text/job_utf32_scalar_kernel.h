#pragma once

#include "job_lattice_kernel.h"
#include "job_unicode_scalar.h"
#include "job_utf32_status.h"

namespace job::core::text {

class Utf32ScalarKernel
{
public:
    using Result     = lattice::LatticeKernelResult<Utf32Status>;
    using UnitInput  = lattice::LatticeKernelInput<char32_t>;
    using UnitOutput = lattice::LatticeKernelOutput<char32_t>;

    Utf32ScalarKernel() = delete;
    ~Utf32ScalarKernel() = delete;

    Utf32ScalarKernel(const Utf32ScalarKernel &) = delete;
    Utf32ScalarKernel &operator=(const Utf32ScalarKernel &) = delete;
    Utf32ScalarKernel(Utf32ScalarKernel &&) = delete;
    Utf32ScalarKernel &operator=(Utf32ScalarKernel &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, UnicodeScalar &output) noexcept
    {
        if (input.empty())
            return {0, 0, Utf32Status::Incomplete};

        const auto scalar = UnicodeScalar::from(input[0]);

        if (!scalar.has_value())
            return {0, 0, Utf32Status::Invalid};

        output = *scalar;
        return {1, 1, Utf32Status::Complete};
    }

    [[nodiscard]] static constexpr Result encode(UnicodeScalar input, UnitOutput output) noexcept
    {
        if (output.empty())
            return {0, 0, Utf32Status::OutputFull};

        output[0] = input.value();
        return {1, 1, Utf32Status::Complete};
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        UnicodeScalar scalar{};
        return decode(input, scalar);
    }
};

} // namespace job::core::text