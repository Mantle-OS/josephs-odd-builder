#pragma once

#include <cstddef>
#include <cstdint>

#include "job_lattice_codec.h"
#include "job_lattice_transform_result.h"
#include "job_lattice_view.h"

#include "job_unicode_scalar.h"
#include "job_utf16_kernel.h"
#include "job_utf16_simd_kernel.h"
#include "job_utf16_status.h"

namespace job::core::text {

class Utf16
{
public:
    using Result       = Utf16Kernel::Result;
    using UnitInput    = Utf16Kernel::UnitInput;
    using UnitOutput   = Utf16Kernel::UnitOutput;
    using ScalarInput  = Utf16Kernel::ScalarInput;
    using ScalarOutput = Utf16Kernel::ScalarOutput;

    Utf16() = delete;
    ~Utf16() = delete;

    Utf16(const Utf16 &) = delete;
    Utf16 &operator=(const Utf16 &) = delete;
    Utf16(Utf16 &&) = delete;
    Utf16 &operator=(Utf16 &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        return Utf16Kernel::decode(input, output);
    }

    [[nodiscard]] static constexpr Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        return Utf16Kernel::encode(input, output);
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        if consteval {
            return Utf16Kernel::validate(input);
        } else {
            if (useSimdValidate(input.size()))
                return Utf16SimdKernel::validate(input);

            return Utf16Kernel::validate(input);
        }
    }

private:
    [[nodiscard]] static constexpr bool useSimdValidate(std::size_t inputSize) noexcept
    {
        return inputSize >= Utf16SimdKernel::kMinimumInputUnits;
    }
};

static_assert(lattice::LatticeCodec<Utf16>);
static_assert(lattice::LatticeCodec<Utf16Kernel>);
static_assert(lattice::LatticeCodec<Utf16SimdKernel>);

} // namespace job::core::text