#pragma once

#include <cstddef>
#include <cstdint>

#include "job_lattice_codec.h"
#include "job_lattice_transform_result.h"
#include "job_lattice_view.h"

#include "job_unicode_scalar.h"
#include "job_utf8_kernel.h"
#include "job_ascii_simd_kernel.h"
#include "job_utf8_simd_kernel.h"
#include "job_utf8_status.h"

namespace job::core::text {

class Utf8
{
public:
    using Result       = Utf8Kernel::Result;
    using UnitInput    = Utf8Kernel::UnitInput;
    using UnitOutput   = Utf8Kernel::UnitOutput;
    using ScalarInput  = Utf8Kernel::ScalarInput;
    using ScalarOutput = Utf8Kernel::ScalarOutput;

    Utf8() = delete;
    ~Utf8() = delete;

    Utf8(const Utf8 &) = delete;
    Utf8 &operator=(const Utf8 &) = delete;
    Utf8(Utf8 &&) = delete;
    Utf8 &operator=(Utf8 &&) = delete;

    [[nodiscard]] static constexpr Result decode(UnitInput input, ScalarOutput output) noexcept
    {
        return Utf8Kernel::decode(input, output);
    }

    [[nodiscard]] static constexpr Result encode(ScalarInput input, UnitOutput output) noexcept
    {
        return Utf8Kernel::encode(input, output);
    }

    [[nodiscard]] static constexpr Result validate(UnitInput input) noexcept
    {
        if consteval {
            return Utf8Kernel::validate(input);
        } else {
            if (useSimdValidate(input.size()))
                return Utf8SimdKernel::validate(input);

            return Utf8Kernel::validate(input);
        }
    }


private:
    //  nice for now
    [[nodiscard]] static constexpr bool useSimdValidate(std::size_t inputSize) noexcept
    {
        return inputSize >= Utf8SimdKernel::kMinimumInputBytes;
    }
};

static_assert(lattice::LatticeCodec<Utf8>);
static_assert(lattice::LatticeCodec<Utf8Kernel>);
static_assert(lattice::LatticeCodec<AsciiSimdKernel>);
static_assert(lattice::LatticeCodec<Utf8SimdKernel>);

} // namespace job::core::text