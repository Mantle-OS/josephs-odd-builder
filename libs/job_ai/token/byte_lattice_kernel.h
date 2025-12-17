#pragma once

#include "byte_lattice.h"

#include <span>
#include <vector>
#include <cassert>

namespace job::ai::token {

struct ByteLatticeKernel {
    // Fast path: caller owns memory.
    static void batchEncode(std::span<const uint8_t> input,
                            std::span<ByteLattice> output,
                            float mass = 1.0f)
    {
        assert(output.size() == input.size());

        for (size_t i = 0; i < input.size(); ++i)
            output[i] = ByteLattice::encode(input[i], mass);
    }

    // Safety dance: resizes output to match input (allocations are explicit by name).
    static void safeBatchEncode(std::span<const uint8_t> input,
                                std::vector<ByteLattice> &output,
                                float mass = 1.0f)
    {
        output.resize(input.size());
        batchEncode(input, std::span<ByteLattice>(output), mass);
    }

    // Fast path: caller owns memory.
    static void batchDecode(std::span<const ByteLattice> input,
                            std::span<uint8_t> output)
    {
        assert(output.size() == input.size());

        for (size_t i = 0; i < input.size(); ++i)
            output[i] = ByteLattice::decode(input[i]);
    }

    // Safety dance: resizes output to match input (allocations are explicit by name).
    static void safeBatchDecode(std::span<const ByteLattice> input,
                                std::vector<uint8_t> &output)
    {
        output.resize(input.size());
        batchDecode(input, std::span<uint8_t>(output));
    }

    // static void batchClampAscii(std::span<const ByteLattice> input, std::vector<uint8_t> &output){}

};

} // namespace job::ai::token
