#pragma once

#include <span>
#include <cassert>
#include <cstdint>
#include <simd_provider.h>

#include "byte_lattice.h"
#include "job_tokenizer_types.h"

#include "jobtoken_export.h"

namespace job::token {

struct JOBTOKEN_EXPORT ByteLatticeKernel {
    // Non-allocating raw byte encode
    static void batchEncodeBytes(std::span<const uint8_t> input, std::span<ByteLattice> output, float mass = 1.0f) noexcept
    {
        assert(output.size() >= input.size());
        const size_t count = std::min(input.size(), output.size());
        for (size_t i = 0; i < count; ++i)
            output[i] = ByteLattice::encodeByte(input[i], mass);
    }

    // Non-allocating raw byte decode
    static void batchDecodeBytes(std::span<const ByteLattice> input, std::span<uint8_t> output) noexcept
    {
        assert(output.size() >= input.size());
        const size_t count = std::min(input.size(), output.size());
        for (size_t i = 0; i < count; ++i)
            output[i] = ByteLattice::decodeByte(input[i]);
    }

    // Non-allocating 21-bit token ID encode
    static void batchEncode21Bit(std::span<const uint32_t> input, std::span<ByteLattice> output, float mass = 1.0f) noexcept
    {
        assert(output.size() >= input.size());
        const size_t count = std::min(input.size(), output.size());
        for (size_t i = 0; i < count; ++i)
            output[i] = ByteLattice::encode21Bit(input[i], mass);
    }

    // Non-allocating 21-bit token ID decode
    static void batchDecode21Bit(std::span<const ByteLattice> input, std::span<uint32_t> output) noexcept
    {
        assert(output.size() >= input.size());
        const size_t count = std::min(input.size(), output.size());
        for (size_t i = 0; i < count; ++i)
            output[i] = ByteLattice::decode21Bit(input[i]);
    }
};

} // namespace job::token