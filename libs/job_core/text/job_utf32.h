#pragma once

#include <cstddef>
#include <cstdint>

#include "job_lattice_codec.h"
#include "job_lattice_transform_result.h"
#include "job_lattice_view.h"

#include "job_unicode_scalar.h"
#include "job_utf32_kernel.h"
#include "job_utf32_simd_kernel.h"
#include "job_utf32_status.h"

namespace job::core::text {

class Utf32
{
public:
    using Result       = Utf32Kernel::Result;
    using UnitInput    = Utf32Kernel::UnitInput;
    using UnitOutput   = Utf32Kernel::UnitOutput;
    using ScalarInput  = Utf32Kernel::ScalarInput;
    using ScalarOutput = Utf32Kernel::ScalarOutput;

    Utf32() = delete;
    ~Utf32() = delete;

    Utf32(const Utf32 &) = delete;
    Utf32 &operator=(const Utf32 &) = delete;
    Utf32(Utf32 &&) = delete;
    Utf32 &operator=(Utf32 &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        return Utf32Kernel::decode(input, output);
    }

    [[nodiscard]] static constexpr Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        return Utf32Kernel::encode(input, output);
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        if consteval {
            return Utf32Kernel::validate(input);
        } else {
            if (useSimdValidate(input.size()))
                return Utf32SimdKernel::validate(input);

            return Utf32Kernel::validate(input);
        }
    }

private:
    [[nodiscard]] static constexpr bool useSimdValidate(std::size_t inputSize) noexcept
    {
        return inputSize >= Utf32SimdKernel::kMinimumInputUnits;
    }
};

static_assert(lattice::LatticeCodec<Utf32>);
static_assert(lattice::LatticeCodec<Utf32Kernel>);
static_assert(lattice::LatticeCodec<Utf32SimdKernel>);

} // namespace job::core::text